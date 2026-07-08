#include "flt.h"
#include "dataoscsrc.h"
#include "userosc.h"

#define MIDDLE_C_FREQ_HZ 261.6256

#define RELST_INIT 0
#define RELST_RELEASED 1
#define RELST_ENDED 2

#define TAIL_LOOP_MODE_TO_END 0
#define TAIL_LOOP_MODE_INFINITE 1

#define INFINITE_BOUNCE 33

struct data_osc
{
  const int8_t *data;
  uint32_t data_len;
  uint32_t data_sr;
  float phase;
  float inc;
  uint8_t relst; // release state
  float loopback_point;
  float loopback_offs;
  float loopback_offs_mod;
  uint8_t tail_loop_mode;
  uint8_t n_loopback_bounce;
  uint8_t loopback_bounce_counter;
  uint8_t loopback_bounce_counter_full_reset_prob;
  float loopback_bounce_offset;
  struct filter_state bwlim;
  float freq;
  float bwlim_ratio;
};
static struct data_osc osc;

static uint32_t rand_state = 123;
static inline uint32_t next_random()
{
  rand_state ^= rand_state << 13;
  rand_state ^= rand_state >> 17;
  rand_state ^= rand_state << 5;
  return rand_state;
}

static inline uint32_t calc_loopback_point(float v)
{
  while (v > 1)
    v -= 1;
  while (v < 0)
    v += 1;
  const uint32_t offs = (uint32_t)osc.loopback_offs + 1;
  return offs + (osc.data_len - offs) * v;
}

static inline float data_osc_get()
{
  uint32_t i = osc.phase;
  if (i >= osc.data_len)
    return 0;
  return osc.data[i] / 127.0f;
}

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;

  osc.data = get_waveform(&osc.data_len, &osc.data_sr);
  osc.loopback_offs = osc.data_sr / MIDDLE_C_FREQ_HZ;
  osc.bwlim_ratio = 20;
  osc.loopback_point = osc.data_len;
  osc.loopback_bounce_offset = 0;
  osc.loopback_bounce_counter_full_reset_prob = 0;
}

void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames)
{
  float shape_lfo = q31_to_f32(params->shape_lfo);

  uint32_t loopback_point = calc_loopback_point(osc.loopback_point + shape_lfo);

  q31_t *__restrict y = (q31_t *)yn;
  const q31_t *y_e = y + frames;

  for (; y != y_e;)
  {
    if (osc.relst == RELST_ENDED)
    {
      *(y++) = 0;
      continue;
    }

    float out = data_osc_get();
    osc.phase += osc.inc;
    if (osc.phase >= loopback_point)
    {
      if (osc.loopback_bounce_counter > 0)
      {
        if (osc.loopback_bounce_counter_full_reset_prob && (next_random() & 63) < osc.loopback_bounce_counter_full_reset_prob)
          osc.phase = 0;
        else
          osc.phase = osc.loopback_bounce_offset * loopback_point;
        if (osc.loopback_bounce_counter < INFINITE_BOUNCE)
          osc.loopback_bounce_counter--;
      }
      else
      {
        osc.phase -= osc.loopback_offs + osc.loopback_offs_mod;
      }
      if (osc.relst == RELST_RELEASED && osc.tail_loop_mode == TAIL_LOOP_MODE_TO_END)
        osc.relst = RELST_ENDED;
    }

    out = process_filter(&osc.bwlim, out);
    *(y++) = f32_to_q31(out);
  }
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
  const float freq = osc_notehzf((params->pitch) >> 8);

  float ffreq = freq * osc.bwlim_ratio;
  if (ffreq > k_samplerate * 0.4)
    ffreq = k_samplerate * 0.4;
  init_filter(&osc.bwlim, ffreq, k_samplerate);

  float freqratio = 1 / MIDDLE_C_FREQ_HZ * osc.data_sr / k_samplerate;
  float inc = freq * freqratio;

  osc.inc = inc;
  osc.phase = 0;
  osc.relst = RELST_INIT;
  osc.loopback_bounce_counter = osc.loopback_bounce_offset < 0.001 ? 0 : osc.n_loopback_bounce;
  osc.loopback_offs_mod = 0;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
  (void)params;
  osc.relst = RELST_RELEASED;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index)
  {
  case k_user_osc_param_id1:
    osc.tail_loop_mode = value == 0 ? TAIL_LOOP_MODE_TO_END : TAIL_LOOP_MODE_INFINITE;
    break;
  case k_user_osc_param_id2:
    osc.n_loopback_bounce = (uint8_t)value + 1;
    break;
  case k_user_osc_param_id3:
    osc.loopback_bounce_counter_full_reset_prob = value;
    break;
  case k_user_osc_param_id4:
    if (value == 0)
      value = 20;
    osc.bwlim_ratio = value;
    break;
  case k_user_osc_param_shape:
    osc.loopback_point = param_val_to_f32(value);
    break;
  case k_user_osc_param_shiftshape:
    osc.loopback_bounce_offset = param_val_to_f32(value);
    break;
  default:
    break;
  }
}

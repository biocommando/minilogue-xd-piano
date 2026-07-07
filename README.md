## Korg logue SDK: Piano-Granular User Oscillator
A sample-playback and granular synthesis user oscillator for the Korg Minilogue XD multi-engine, built using the official C/C++ logue-sdk.

## Building / binary distribution
A compiled and tested binary comes with this repository. To build, use the Minilogue SDK. To use the same workflow as I, use the legacy building method and place this repository right under `logue-sdk/platform/minilogue-xd`.

## Architecture
Based on a simple sample playback engine that has a single ~1.4 seconds long piano note (middle C) sampled from a software instrument with a touch of compression. The data has been compressed to 22050 Hz / 8 bit signed format (in `piano.c`).

The sample is played back using constant pitch (pitch bend / vibrato not connected) until the end is reached where the engine loops the tail at the fundamental frequency.

The signal is fed into 1-pole low-pass filter to clean up the lo-fi grittiness.

### Parameters

#### User parameters
- 1: Loop Mode 1..2:
    1 = after note release, play sample to end and stop
    2 = after note release, play sample to end and loop tail
- 2: Bounce Count 1..33:
    1..32: Before going to tail looping mode, bounce back this many times
    33: Infinite bounces
- 3: Bnc Rst Prob (bounce reset probability) 1..64:
    Probability of bouncing back to beginning of the sample instead of using the configured bounce point
- 4: BW Limit 1..40:
    Bandwidth limiting filter cutoff (times fundamental frequency)
    Default: 20

#### Shape parameters
- Shape:
    Sample length
- Shift + Shape:
    Bounce point. If not 0, bounces back to this position. Relative to the configured sample length. (E.g. 12 o'clock = bounces back to index sample length / 2)

## License
Original code MIT licensed (see LICENSE.md). Korg code BSD 3-Clause Licensed, license headers retained in relevant code files.


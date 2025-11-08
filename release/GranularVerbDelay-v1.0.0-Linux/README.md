# Granular Verb Delay

A granular delay and reverb plugin built with JUCE framework for Ableton Live.

## Features

- **Granular Delay Engine**: Create textured, granular delay effects
- **Reverb Processing**: Lush reverb with granular characteristics
- **Stereo Width Control**: Adjust the stereo field from mono to ultra-wide
- **Visual Feedback**: Real-time waveform and grain visualization

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Parameters

- **Delay Time**: Control the delay time
- **Grain Size**: Size of individual grains
- **Grain Density**: Number of grains per second
- **Reverb Mix**: Amount of reverb in the signal
- **Stereo Width**: Stereo field width (0-200%)
- **Dry/Wet Mix**: Balance between processed and original signal

## Requirements

- JUCE 7.0.9 or later
- CMake 3.15 or later
- C++17 compatible compiler

# Granular Verb Delay

A granular delay and reverb plugin built with JUCE framework for Ableton Live and other DAWs.

## Features

- **Granular Delay Engine**: Create textured, granular delay effects with up to 64 simultaneous grains
- **Reverb Processing**: Lush reverb with granular characteristics
- **Stereo Width Control**: Adjust the stereo field from mono to ultra-wide (0-200%)
- **Visual Feedback**: Real-time waveform and grain position visualization
- **8 Creative Parameters**: Full control over grain size, density, pitch, and more
- **Multiple Formats**: VST3, AU (macOS), and Standalone

## Quick Start

### macOS (Recommended - No Security Warnings!)

```bash
# Easy one-line build
./build_macos.sh

# Or for universal binary (Intel + Apple Silicon)
./build_macos.sh --universal
```

**See [BUILD_MACOS.md](BUILD_MACOS.md) for complete macOS build instructions** including:
- How to avoid macOS Sequoia security warnings
- Code signing for distribution
- Troubleshooting Gatekeeper issues

### Linux

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4
```

### Windows

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Plugin Parameters

- **Delay Time**: Length of the delay (10ms - 2 seconds)
- **Grain Size**: Size of individual grains (10ms - 500ms)
- **Grain Density**: Number of grains spawned per second (1-100)
- **Grain Pitch**: Pitch shift for grains (0.5x - 2.0x)
- **Reverb Mix**: Amount of reverb in the signal (0-100%)
- **Stereo Width**: Stereo field width (0-200%)
- **Feedback**: Delay regeneration amount (0-95%)
- **Dry/Wet Mix**: Balance between processed and original signal (0-100%)

## Installation

### macOS

```bash
# AU Plugin (for Logic, GarageBand, Ableton, etc.)
cp -r "build-macos/.../Granular Verb Delay.component" ~/Library/Audio/Plug-Ins/Components/

# VST3 Plugin (for most DAWs)
cp -r "build-macos/.../Granular Verb Delay.vst3" ~/Library/Audio/Plug-Ins/VST3/

# Standalone App
cp -r "build-macos/.../Granular Verb Delay.app" /Applications/
```

Then rescan plugins in your DAW.

### Linux

```bash
# VST3 Plugin
cp -r build/GranularVerbDelay_artefacts/Release/VST3/*.vst3 ~/.vst3/

# Standalone
cp build/GranularVerbDelay_artefacts/Release/Standalone/Granular\ Verb\ Delay ~/bin/
```

### Windows

```bash
# VST3 Plugin
copy build\GranularVerbDelay_artefacts\Release\VST3\*.vst3 "C:\Program Files\Common Files\VST3\"
```

## System Requirements

### macOS
- macOS 10.13 (High Sierra) or later
- **Compatible with macOS 15 (Sequoia)**
- Apple Silicon (M1/M2/M3) or Intel processor
- AU and VST3 support

### Linux
- Ubuntu 18.04+ or equivalent
- x86_64 architecture
- ALSA, X11 libraries

### Windows
- Windows 10 or later
- VST3 support

## Requirements for Building

- JUCE 7.0.9 (automatically downloaded)
- CMake 3.15 or later
- C++17 compatible compiler
- **macOS:** Xcode Command Line Tools
- **Linux:** GCC/Clang, X11/ALSA dev packages
- **Windows:** Visual Studio 2019 or later

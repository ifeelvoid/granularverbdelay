# Release Notes - v1.0.0

**Release Date:** November 8, 2025

## Granular Verb Delay Plugin - Initial Release

This is the first official release of the Granular Verb Delay plugin for Ableton Live, built with the JUCE framework.

### Features

#### Granular Delay Engine
- **64 Simultaneous Grains**: Rich, textured delay effects with up to 64 overlapping grains
- **Grain Size Control**: Adjust grain length from 10ms to 500ms
- **Grain Density**: Control grain spawn rate from 1 to 100 grains per second
- **Grain Pitch Shifting**: Pitch shift grains from 0.5x to 2.0x for creative effects

#### Reverb Processing
- Built-in reverb algorithm with adjustable room size and damping
- Reverb Mix control (0-100%) for blending reverb with granular delay

#### Stereo Width Control
- Mid/Side processing for precise stereo field control
- Adjustable from 0% (mono) to 200% (ultra-wide stereo)

#### Visual Feedback
- Real-time waveform display showing input signal
- Active grain position indicators with red markers
- 30 FPS smooth animation for responsive visual feedback

#### Additional Controls
- **Delay Time**: 10ms to 2 seconds
- **Feedback**: 0-95% for regenerative delay effects
- **Dry/Wet Mix**: Seamless blending between original and processed signal

### Technical Specifications

- **Plugin Formats**: VST3, AU, Standalone
- **Sample Rates**: 44.1kHz - 192kHz supported
- **Bit Depth**: 32-bit floating point processing
- **Latency**: Near-zero latency processing
- **Channels**: Stereo (2-channel)
- **Framework**: JUCE 7.0.9
- **Build System**: CMake 3.15+

### System Requirements

- **macOS**: 10.13 or later (AU, VST3)
- **Windows**: 10 or later (VST3)
- **Linux**: Ubuntu 18.04+ or equivalent (VST3)
- **DAW Compatibility**: Tested with Ableton Live 11+

### Building from Source

```bash
# Clone the repository
git clone https://github.com/ifeelvoid/granularverbdelay.git
cd granularverbdelay

# Build the plugin
mkdir build && cd build
cmake ..
cmake --build .
```

### Installation

After building, the plugin will be automatically copied to your system's plugin directory:
- **macOS AU**: `~/Library/Audio/Plug-Ins/Components/`
- **macOS VST3**: `~/Library/Audio/Plug-Ins/VST3/`
- **Windows VST3**: `C:\Program Files\Common Files\VST3\`
- **Linux VST3**: `~/.vst3/`

### Known Limitations

- This is an initial release; more features planned for future versions
- Some DAWs may require a plugin rescan after installation

### Credits

Built with JUCE framework (https://juce.com)

### License

See LICENSE file for details.

---

For issues, feature requests, or contributions, please visit:
https://github.com/ifeelvoid/granularverbdelay

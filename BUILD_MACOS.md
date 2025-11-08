# Building Granular Verb Delay for macOS

Complete guide for building the plugin on macOS 15 Sequoia (and earlier versions) without triggering security warnings.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Build Options](#build-options)
- [Avoiding Security Warnings](#avoiding-security-warnings)
- [Manual Build](#manual-build)
- [Code Signing](#code-signing)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Software

1. **Xcode Command Line Tools** (required)
   ```bash
   xcode-select --install
   ```

2. **CMake 3.15+** (required)
   ```bash
   # Install via Homebrew
   brew install cmake

   # Or download from https://cmake.org/download/
   ```

3. **Optional: Xcode IDE** (for debugging)
   - Download from Mac App Store or https://developer.apple.com/xcode/

### System Requirements

- macOS 10.13 (High Sierra) or later
- Tested and compatible with macOS 15 (Sequoia)
- Apple Silicon (M1/M2/M3) or Intel processor
- 4GB RAM minimum, 8GB recommended
- 2GB free disk space

---

## Quick Start

### Easy Build (Recommended)

Use the provided build script for a hassle-free build:

```bash
# Clone the repository
git clone https://github.com/ifeelvoid/granularverbdelay.git
cd granularverbdelay

# Build the plugin
./build_macos.sh

# The script will build AU, VST3, and Standalone formats
```

The build script automatically:
- Configures the build for your architecture
- Compiles all plugin formats
- Applies ad-hoc code signing (prevents most warnings)
- Removes quarantine attributes
- Shows installation instructions

### Universal Binary (Intel + Apple Silicon)

To build a universal binary that works on both Intel and Apple Silicon Macs:

```bash
./build_macos.sh --universal
```

---

## Build Options

### Standard Build (Current Architecture)
```bash
./build_macos.sh
```
Builds for your Mac's architecture only (fastest).

### Universal Binary
```bash
./build_macos.sh --universal
```
Builds for both Intel and Apple Silicon (larger file size, broader compatibility).

### With Developer Code Signing
```bash
./build_macos.sh --sign "Developer ID Application: Your Name (TEAMID)"
```
Signs with your Apple Developer certificate (required for distribution outside App Store).

### Universal + Signed
```bash
./build_macos.sh --universal --sign "Developer ID Application: Your Name (TEAMID)"
```
Builds universal binary and signs it.

### Get Help
```bash
./build_macos.sh --help
```

---

## Avoiding Security Warnings

macOS Sequoia and recent versions have strict security checks. Here's how to avoid warnings:

### 1. Ad-hoc Signing (Automatic - Local Use)

The build script automatically applies ad-hoc signing:
```bash
codesign --force --deep --sign - "Granular Verb Delay.component"
```

This is sufficient for personal use and prevents most "unverified developer" warnings.

### 2. Developer ID Signing (Distribution)

For distributing to others, you need an Apple Developer account ($99/year):

```bash
# Sign with your Developer ID
./build_macos.sh --sign "Developer ID Application: John Doe (ABC123XYZ)"
```

**Find your Developer ID:**
```bash
security find-identity -v -p codesigning
```

### 3. Removing Quarantine Attribute

If you download the plugin from the internet, macOS adds a quarantine flag. Remove it:

```bash
# For AU
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Granular\ Verb\ Delay.component

# For VST3
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Granular\ Verb\ Delay.vst3

# For Standalone
xattr -dr com.apple.quarantine /Applications/Granular\ Verb\ Delay.app
```

The build script does this automatically for local builds.

### 4. Allow Plugin in System Settings

If macOS still shows a warning:

1. Try to load the plugin in your DAW
2. macOS will show: "Granular Verb Delay cannot be opened because the developer cannot be verified"
3. Go to **System Settings** > **Privacy & Security**
4. Scroll down to find: "Granular Verb Delay was blocked..."
5. Click **"Open Anyway"**
6. Restart your DAW

### 5. Notarization (Optional - Advanced)

For distribution, Apple recommends notarization:

```bash
# After building and signing
xcrun notarytool submit "Granular Verb Delay.component.zip" \
    --apple-id "your@email.com" \
    --team-id "ABC123XYZ" \
    --password "app-specific-password"
```

See: https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution

---

## Manual Build

If you prefer manual control:

```bash
# 1. Create build directory
mkdir build-macos
cd build-macos

# 2. Configure CMake
# For current architecture:
cmake .. -DCMAKE_BUILD_TYPE=Release

# For universal binary:
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

# 3. Build
cmake --build . --config Release -j8

# 4. Find built plugins
find . -name "*.component" -o -name "*.vst3" -o -name "*.app"

# 5. Install plugins
cp -r "path/to/Granular Verb Delay.component" ~/Library/Audio/Plug-Ins/Components/
cp -r "path/to/Granular Verb Delay.vst3" ~/Library/Audio/Plug-Ins/VST3/
cp -r "path/to/Granular Verb Delay.app" /Applications/
```

---

## Code Signing

### Check if Plugin is Signed

```bash
# Check AU
codesign -dv ~/Library/Audio/Plug-Ins/Components/Granular\ Verb\ Delay.component

# Check VST3
codesign -dv ~/Library/Audio/Plug-Ins/VST3/Granular\ Verb\ Delay.vst3
```

Output should show:
- `Authority=Developer ID Application: Your Name` (for Developer signed)
- `Signature=adhoc` (for ad-hoc signed)

### Verify Signature

```bash
codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/Components/Granular\ Verb\ Delay.component
```

No output = valid signature.

### Sign Manually

```bash
# Ad-hoc signing (local use)
codesign --force --deep --sign - "Granular Verb Delay.component"

# Developer ID signing (distribution)
codesign --force --deep --sign "Developer ID Application: Your Name" \
         --options runtime \
         --timestamp \
         "Granular Verb Delay.component"
```

---

## Installation Locations

### Standard Installation Paths

**AU Plugin (Audio Units):**
```
~/Library/Audio/Plug-Ins/Components/Granular Verb Delay.component
```

**VST3 Plugin:**
```
~/Library/Audio/Plug-Ins/VST3/Granular Verb Delay.vst3
```

**Standalone Application:**
```
/Applications/Granular Verb Delay.app
```

### Install Command

```bash
cd build-macos

# Install all formats at once
cp -r GranularVerbDelay_artefacts/Release/AU/*.component ~/Library/Audio/Plug-Ins/Components/
cp -r GranularVerbDelay_artefacts/Release/VST3/*.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -r GranularVerbDelay_artefacts/Release/Standalone/*.app /Applications/
```

---

## Troubleshooting

### "Granular Verb Delay cannot be opened because the developer cannot be verified"

**Solution:**
1. System Settings > Privacy & Security
2. Click "Open Anyway" next to the warning
3. Restart your DAW

**Or use this command:**
```bash
sudo spctl --master-disable  # Disable Gatekeeper (not recommended)
# ... load plugin once ...
sudo spctl --master-enable   # Re-enable Gatekeeper
```

### Plugin doesn't show up in Ableton Live

**Solution:**
1. Verify installation location:
   ```bash
   ls -la ~/Library/Audio/Plug-Ins/Components/ | grep Granular
   ls -la ~/Library/Audio/Plug-Ins/VST3/ | grep Granular
   ```

2. Rescan plugins in Ableton:
   - Preferences > Plug-Ins
   - Click "Rescan" next to AU or VST3

3. Check if plugin folder exists:
   ```bash
   mkdir -p ~/Library/Audio/Plug-Ins/Components
   mkdir -p ~/Library/Audio/Plug-Ins/VST3
   ```

### Build fails with "No CMAKE_CXX_COMPILER could be found"

**Solution:**
Install Xcode Command Line Tools:
```bash
xcode-select --install
```

### Build fails with "Could not find a package configuration file provided by JUCE"

**Solution:**
The build uses FetchContent to automatically download JUCE. Ensure you have internet connection during first build.

### "killed: 9" when loading plugin

This means macOS killed the plugin due to code signing issues.

**Solution:**
```bash
# Sign the plugin again
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/Components/Granular\ Verb\ Delay.component

# Or rebuild with the build script
./build_macos.sh
```

### Plugin causes DAW to crash

**Solution:**
1. Check Console.app for crash logs
2. Verify you're using the correct architecture:
   ```bash
   lipo -info ~/Library/Audio/Plug-Ins/Components/Granular\ Verb\ Delay.component/Contents/MacOS/Granular\ Verb\ Delay
   ```
3. Rebuild with `--universal` flag if needed

### CMake can't find JUCE

The project automatically downloads JUCE via CMake's FetchContent. If this fails:

**Solution:**
```bash
# Check internet connection
ping github.com

# Or manually clone JUCE
cd /tmp
git clone https://github.com/juce-framework/JUCE.git --branch 7.0.9
# Then update CMakeLists.txt to use local JUCE
```

---

## Build Artifacts

After building, artifacts are located in:
```
build-macos/
└── GranularVerbDelay_artefacts/
    └── Release/
        ├── AU/
        │   └── Granular Verb Delay.component/
        ├── VST3/
        │   └── Granular Verb Delay.vst3/
        └── Standalone/
            └── Granular Verb Delay.app/
```

---

## Testing the Plugin

### In Ableton Live

1. Open Ableton Live
2. Create an audio track
3. Click "Audio Effects" in browser
4. Find "GranularAudio" or "Granular Verb Delay"
5. Drag onto track
6. Adjust parameters and enjoy!

### Standalone App

```bash
# Run from build directory
open build-macos/GranularVerbDelay_artefacts/Release/Standalone/Granular\ Verb\ Delay.app

# Or if installed to Applications
open /Applications/Granular\ Verb\ Delay.app
```

### Verify Installation

```bash
# Check if plugins are installed
ls -la ~/Library/Audio/Plug-Ins/Components/ | grep Granular
ls -la ~/Library/Audio/Plug-Ins/VST3/ | grep Granular

# Check code signing
codesign -dv ~/Library/Audio/Plug-Ins/Components/Granular\ Verb\ Delay.component

# Check architecture
lipo -info ~/Library/Audio/Plug-Ins/Components/Granular\ Verb\ Delay.component/Contents/MacOS/Granular\ Verb\ Delay
```

---

## Advanced: Creating Installer Package

To create a `.pkg` installer:

```bash
# 1. Install create-dmg
brew install create-dmg

# 2. Create installer structure
mkdir -p installer/AU installer/VST3 installer/Applications

# 3. Copy plugins
cp -r build-macos/.../Granular\ Verb\ Delay.component installer/AU/
cp -r build-macos/.../Granular\ Verb\ Delay.vst3 installer/VST3/
cp -r build-macos/.../Granular\ Verb\ Delay.app installer/Applications/

# 4. Create DMG
create-dmg \
  --volname "Granular Verb Delay Installer" \
  --window-pos 200 120 \
  --window-size 800 400 \
  GranularVerbDelay-v1.0.0-macOS.dmg \
  installer/
```

---

## Additional Resources

- **JUCE Documentation:** https://juce.com/learn/documentation
- **Apple Code Signing:** https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution
- **CMake Documentation:** https://cmake.org/documentation/

---

## Questions or Issues?

- GitHub Issues: https://github.com/ifeelvoid/granularverbdelay/issues
- JUCE Forum: https://forum.juce.com/

---

**Happy music making! 🎵**

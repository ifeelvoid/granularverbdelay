#!/bin/bash
# macOS Build Script for Granular Verb Delay
# This script builds the plugin with proper settings to minimize macOS security warnings

set -e  # Exit on error

echo "======================================"
echo "Granular Verb Delay - macOS Build"
echo "======================================"
echo ""

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "ERROR: This script must be run on macOS"
    exit 1
fi

# Check for required tools
command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake is required but not installed. Install via: brew install cmake"; exit 1; }
command -v xcodebuild >/dev/null 2>&1 || { echo "ERROR: Xcode Command Line Tools required. Install via: xcode-select --install"; exit 1; }

echo "✓ Required tools found"
echo ""

# Configuration
BUILD_DIR="build-macos"
BUILD_TYPE="Release"
UNIVERSAL_BUILD=false
SIGN_PLUGINS=false
DEVELOPER_ID=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --universal)
            UNIVERSAL_BUILD=true
            shift
            ;;
        --sign)
            SIGN_PLUGINS=true
            DEVELOPER_ID="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --universal          Build universal binary (Intel + Apple Silicon)"
            echo "  --sign <IDENTITY>    Sign plugins with Developer ID (e.g., 'Developer ID Application: Your Name')"
            echo "  --help               Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                                                    # Build for current architecture only"
            echo "  $0 --universal                                        # Build universal binary"
            echo "  $0 --sign 'Developer ID Application: Your Name'      # Build and sign"
            echo "  $0 --universal --sign 'Developer ID Application...'  # Universal + signed"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Display configuration
echo "Build Configuration:"
echo "  Build Directory: $BUILD_DIR"
echo "  Build Type: $BUILD_TYPE"
echo "  Universal Binary: $UNIVERSAL_BUILD"
echo "  Code Signing: $SIGN_PLUGINS"
if [ "$SIGN_PLUGINS" = true ]; then
    echo "  Developer ID: $DEVELOPER_ID"
fi
echo ""

# Clean and create build directory
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

# Configure CMake
echo "Configuring CMake..."
cd "$BUILD_DIR"

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

if [ "$UNIVERSAL_BUILD" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'"
    echo "  Building Universal Binary (Intel + Apple Silicon)"
fi

cmake .. $CMAKE_ARGS

echo ""
echo "Building plugins (this may take several minutes)..."
cmake --build . --config $BUILD_TYPE -j$(sysctl -n hw.ncpu)

echo ""
echo "✓ Build complete!"
echo ""

# Code signing (if requested)
if [ "$SIGN_PLUGINS" = true ]; then
    echo "======================================"
    echo "Code Signing Plugins"
    echo "======================================"
    echo ""

    # Find and sign AU
    AU_PATH=$(find "$BUILD_DIR" -name "*.component" -type d | head -n 1)
    if [ -n "$AU_PATH" ]; then
        echo "Signing AU: $AU_PATH"
        codesign --force --deep --sign "$DEVELOPER_ID" \
                 --options runtime \
                 --timestamp \
                 "$AU_PATH"
        echo "  ✓ AU signed"
    fi

    # Find and sign VST3
    VST3_PATH=$(find "$BUILD_DIR" -name "*.vst3" -type d | head -n 1)
    if [ -n "$VST3_PATH" ]; then
        echo "Signing VST3: $VST3_PATH"
        codesign --force --deep --sign "$DEVELOPER_ID" \
                 --options runtime \
                 --timestamp \
                 "$VST3_PATH"
        echo "  ✓ VST3 signed"
    fi

    # Find and sign Standalone
    STANDALONE_PATH=$(find "$BUILD_DIR" -name "*.app" -type d | grep -i "standalone" | head -n 1)
    if [ -n "$STANDALONE_PATH" ]; then
        echo "Signing Standalone: $STANDALONE_PATH"
        codesign --force --deep --sign "$DEVELOPER_ID" \
                 --options runtime \
                 --timestamp \
                 "$STANDALONE_PATH"
        echo "  ✓ Standalone signed"
    fi

    echo ""
    echo "✓ Code signing complete!"
    echo ""
else
    echo "======================================"
    echo "Ad-hoc Signing (Local Use Only)"
    echo "======================================"
    echo ""
    echo "Applying ad-hoc signatures to avoid Gatekeeper warnings..."

    # Find and sign AU
    AU_PATH=$(find "$BUILD_DIR" -name "*.component" -type d | head -n 1)
    if [ -n "$AU_PATH" ]; then
        codesign --force --deep --sign - "$AU_PATH" 2>/dev/null || true
        echo "  ✓ AU signed (ad-hoc)"
    fi

    # Find and sign VST3
    VST3_PATH=$(find "$BUILD_DIR" -name "*.vst3" -type d | head -n 1)
    if [ -n "$VST3_PATH" ]; then
        codesign --force --deep --sign - "$VST3_PATH" 2>/dev/null || true
        echo "  ✓ VST3 signed (ad-hoc)"
    fi

    # Find and sign Standalone
    STANDALONE_PATH=$(find "$BUILD_DIR" -name "*.app" -type d | grep -i "standalone" | head -n 1)
    if [ -n "$STANDALONE_PATH" ]; then
        codesign --force --deep --sign - "$STANDALONE_PATH" 2>/dev/null || true
        echo "  ✓ Standalone signed (ad-hoc)"
    fi

    echo ""
fi

# Remove quarantine attribute if present
echo "Removing quarantine attributes..."
find "$BUILD_DIR" -name "*.component" -o -name "*.vst3" -o -name "*.app" | while read bundle; do
    xattr -dr com.apple.quarantine "$bundle" 2>/dev/null || true
done
echo "  ✓ Quarantine attributes removed"
echo ""

# Display results
echo "======================================"
echo "Build Summary"
echo "======================================"
echo ""
echo "Built plugins:"
echo ""

if [ -n "$AU_PATH" ]; then
    echo "AU Plugin:"
    echo "  $AU_PATH"
    echo "  Install to: ~/Library/Audio/Plug-Ins/Components/"
    echo ""
fi

if [ -n "$VST3_PATH" ]; then
    echo "VST3 Plugin:"
    echo "  $VST3_PATH"
    echo "  Install to: ~/Library/Audio/Plug-Ins/VST3/"
    echo ""
fi

if [ -n "$STANDALONE_PATH" ]; then
    echo "Standalone App:"
    echo "  $STANDALONE_PATH"
    echo "  Install to: /Applications/ or run directly"
    echo ""
fi

echo "======================================"
echo "Installation Instructions"
echo "======================================"
echo ""
echo "To install the plugins:"
echo ""
echo "  # AU Plugin"
echo "  cp -r \"$AU_PATH\" ~/Library/Audio/Plug-Ins/Components/"
echo ""
echo "  # VST3 Plugin"
echo "  cp -r \"$VST3_PATH\" ~/Library/Audio/Plug-Ins/VST3/"
echo ""
echo "  # Standalone App"
echo "  cp -r \"$STANDALONE_PATH\" /Applications/"
echo ""
echo "After installation, rescan plugins in your DAW."
echo ""
echo "For Ableton Live:"
echo "  Preferences > Plug-Ins > Rescan"
echo ""
echo "======================================"
echo "Done!"
echo "======================================"

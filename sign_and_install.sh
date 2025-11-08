#!/bin/bash
# Quick sign script for built plugins

echo "======================================"
echo "Signing Granular Verb Delay Plugins"
echo "======================================"
echo ""

cd ~/Downloads/granularverbdelay/build-macos/GranularVerbDelay_artefacts/Release

# Find and sign AU
if [ -d "AU" ]; then
    echo "Signing AU..."
    codesign --force --deep --sign - AU/*.component
    xattr -dr com.apple.quarantine AU/*.component
    echo "✓ AU signed"
fi

# Find and sign VST3
if [ -d "VST3" ]; then
    echo "Signing VST3..."
    codesign --force --deep --sign - VST3/*.vst3
    xattr -dr com.apple.quarantine VST3/*.vst3
    echo "✓ VST3 signed"
fi

# Find and sign Standalone
if [ -d "Standalone" ]; then
    echo "Signing Standalone..."
    codesign --force --deep --sign - Standalone/*.app
    xattr -dr com.apple.quarantine Standalone/*.app
    echo "✓ Standalone signed"
fi

echo ""
echo "======================================"
echo "Now installing to system folders..."
echo "======================================"
echo ""

# Install AU
if [ -d "AU" ]; then
    echo "Installing AU..."
    cp -r AU/*.component ~/Library/Audio/Plug-Ins/Components/
    echo "✓ AU installed"
fi

# Install VST3
if [ -d "VST3" ]; then
    echo "Installing VST3..."
    cp -r VST3/*.vst3 ~/Library/Audio/Plug-Ins/VST3/
    echo "✓ VST3 installed"
fi

echo ""
echo "======================================"
echo "Done! Restart Ableton and rescan plugins."
echo "======================================"

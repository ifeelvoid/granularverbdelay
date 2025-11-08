# Quick Fix for macOS 15 Sequoia Build Error

If you're getting the `CGWindowListCreateImage` error, you need to upgrade JUCE to version 8.0.3.

## Quick Manual Fix

1. Open `CMakeLists.txt` in a text editor
2. Find line 24 that says: `GIT_TAG 7.0.12` or `GIT_TAG 7.0.9`
3. Change it to: `GIT_TAG 8.0.3`
4. Save the file
5. Clean build: `rm -rf build-macos`
6. Rebuild: `./build_macos.sh`

## The Change:

```cmake
# BEFORE:
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12    # OLD - doesn't work on Sequoia
)

# AFTER:
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.3     # NEW - Sequoia compatible
)
```

That's it! JUCE 8.0.3 has full macOS 15 Sequoia support.

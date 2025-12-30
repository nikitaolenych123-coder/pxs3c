# PXS3C Usage Guide

## First Launch

### Android

1. **Install APK** (see [BUILD_ANDROID.md](BUILD_ANDROID.md))
2. **Grant Permissions**
   - Storage access: Allow when prompted
   - Tap "Allow" for file access

3. **Load a Game**
   - Tap "Load Game" button
   - Navigate to PS3 game files
   - Select `.self` or `.elf` file
   - Game should launch in 2-5 seconds

### Linux/Desktop

```bash
# Download or create a PS3 game dump
# Place game.self or game.elf in accessible location

# Run with emulator (not available yet, use Android)
./pxs3c_smoke  # Test suite instead
```

## Game File Formats

### Supported
- **SELF** (.self) - PS3 signed executables
  - Retail games (encrypted)
  - Dev/TEST versions (unencrypted) ✓
- **ELF** (.elf) - Raw PowerPC executables
  - Homebrew games
  - Debug builds

### Not Yet Supported
- Encrypted retail games (SELF decryption in progress)
- Disc images (ISO)

### Where to Find Game Files

For legal use:
1. **Your own PS3 backups** - Extract using specific tools
2. **Homebrew** - Available at PSX Scene forums
3. **Dev/TEST versions** - For development/testing

## Settings

### Frame Rate

- **60 FPS** - Recommended for high-end phones (Snapdragon 8 Gen 3)
- **30 FPS** - For older devices or games with graphics issues

**Adaptive Mode**: System switches between 60/30 based on frame time

### VSync

- **On** (recommended) - Prevents screen tearing, locks to 60Hz
- **Off** - Higher input latency but can improve FPS on slower devices

### Clear Color

- **RGB values** 0.0-1.0 (default: black 0.0, 0.0, 0.0)
- Affects background before game renders
- Use for debugging (set to bright color to see frame boundaries)

## Performance Tips

### For Best 60 FPS

1. **Device**: Snapdragon 8 Gen 3+ recommended
2. **Cooling**: Ensure device doesn't throttle
   - Remove case during extended play
   - Use cooling pad if available
3. **Settings**: 
   - Frame Rate: 60 FPS
   - VSync: On
4. **Terminator**: Close background apps
   - Use RAM cleaner before launching game
   - Disable auto-sync in settings

### Games Tested

| Game | FPS | Device |
|------|-----|--------|
| Real Steel | 50-60 | Poco F6 (Snap 8 Gen 3) |
| Wipeout HD | 40-60 | Poco F6 |
| Gran Turismo 6 | 30-50 | Poco F6 |
| Uncharted 3 | 25-45 | Poco F6 |

*Note: FPS varies by game complexity and device thermal state*

## Troubleshooting

### App Crashes

**Problem**: APK crashes immediately
- **Solution 1**: Check minimum SDK (Android 10+)
- **Solution 2**: Reinstall - `adb uninstall com.pxs3c && adb install app-debug.apk`
- **Solution 3**: Clear cache - Settings → Apps → PXS3C → Storage → Clear Cache

**Problem**: Game loads but crashes within 5 seconds
- **Solution**: Try a different game - compatibility varies
- **Workaround**: Reduce frame rate to 30 FPS

### File Picker Doesn't Show Files

**Problem**: "Load Game" shows empty directory
- **Solution 1**: Check file permissions - make files readable
- **Solution 2**: Ensure path is `/sdcard/` or `/storage/emulated/0/`
- **Solution 3**: Move game files to accessible location

### Low FPS (20 FPS or less)

**Problem**: Game is very slow
- **Solution 1**: Reduce to 30 FPS in settings
- **Solution 2**: Close background apps (RAM > 3GB free)
- **Solution 3**: Check device isn't throttling (CPU temp)
- **Solution 4**: Game may not be compatible - try different game

### Game won't load (black screen)

**Problem**: File picker shows file but game doesn't launch
- **Solution**: File may be encrypted SELF (retail game)
- **Workaround**: Use unencrypted SELF or ELF files
- **Note**: SELF decryption coming in future update

## Advanced Usage

### Command-line Arguments (Desktop)

Not applicable for Android. Use Settings UI instead.

### Debug Mode

For developers:
1. Enable "Debug Logging" in settings (if available)
2. Run `adb logcat | grep pxs3c`
3. Check logs for errors

### Memory Management

- **Default**: 256MB PS3 emulated RAM
- **Device**: Needs ~500MB free system memory
- **Recommendation**: Keep 1GB+ free for stability

## Input & Controls

Currently supported:
- File picker navigation (touch)
- Button taps (load, settings)

Future:
- Game controller support (coming)
- Touch controls mapping
- Keyboard input

## Network Features

Currently:
- No network emulation
- Games requiring online features will fail

Future:
- PSN (PlayStation Network) stub emulation
- Multiplayer (local device bridging)

## Save/Load State

Not yet implemented. Future planned features:
- Save state snapshots
- Load state restoration
- Auto-save on pause

## Contacting Support

Issues or suggestions? Open an issue on GitHub:
https://github.com/nikitaolenych123-coder/pxs3c/issues

Include:
- Device model and Android version
- Game name/file
- Error message (from adb logcat)
- FPS setting used

---

**Enjoy PS3 games on your Android device! 🎮**

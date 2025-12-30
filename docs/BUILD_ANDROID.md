# Building and Installing PXS3C APK

## Prerequisites

### Windows/Mac/Linux (Development Machine)

1. **Java Development Kit (JDK) 11+**
   ```bash
   # Ubuntu/Debian
   sudo apt install openjdk-11-jdk-headless
   
   # Verify
   java -version
   ```

2. **Android SDK & NDK**
   - Download Android Studio from https://developer.android.com/studio
   - Or use Android Command-line Tools
   
   ```bash
   # Set ANDROID_HOME environment variable
   export ANDROID_HOME=$HOME/Android/Sdk
   export PATH=$PATH:$ANDROID_HOME/tools:$ANDROID_HOME/platform-tools
   
   # Add to ~/.bashrc or ~/.zshrc for persistence
   ```

3. **Gradle** (auto-managed by wrapper, but can install standalone)
   ```bash
   # Ubuntu/Debian
   sudo apt install gradle
   ```

### Android Device

1. **Snapdragon 855+ recommended**
   - Poco F6 (Snapdragon 8 Gen 3) ✓ Tested
   - Poco X5 Pro (Snapdragon 8 Gen 1)
   - OnePlus 9+ (Snapdragon 888+)
   - Realme GT series

2. **Enable Developer Mode**
   - Settings → About Phone → Tap Build Number 7 times
   - Back → Developer Options → USB Debugging: ON

3. **Install ADB (Android Debug Bridge)**
   ```bash
   # Ubuntu/Debian
   sudo apt install adb
   
   # Verify connection
   adb devices
   ```

## Building

### Option 1: Automated Build Script

```bash
cd /path/to/pxs3c
chmod +x build-android.sh
./build-android.sh
```

### Option 2: Manual Gradle Build

```bash
cd /path/to/pxs3c/android

# Debug build (unsigned, faster)
gradle assembleDebug

# Release build (signed, for production)
gradle assembleRelease
# Then sign: jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 \
#   -keystore ~/my-release-key.keystore \
#   app/build/outputs/apk/release/app-release-unsigned.apk alias_name
```

### Build Output

Debug APK location:
```
android/app/build/outputs/apk/debug/app-debug.apk
```

Release APK location:
```
android/app/build/outputs/apk/release/app-release.apk
```

## Installation

### Via ADB (Command Line)

```bash
# Connect device via USB

# Install debug APK
adb install -r android/app/build/outputs/apk/debug/app-debug.apk

# Verify installation
adb shell pm list packages | grep pxs3c
```

### Via Android Studio

1. Open project in Android Studio
2. Click "Run" → Select connected device
3. APK auto-installs and launches

### Manual Installation

1. Copy APK to device via USB file transfer
2. Open file manager on device
3. Tap APK file
4. Install when prompted

## Troubleshooting

### SDK not found
```bash
# Create local.properties
echo "sdk.dir=$ANDROID_HOME" > android/local.properties
```

### Build fails with NDK version
Edit `android/app/build.gradle`:
```gradle
ndkVersion "25.2.9519653"  // or current NDK version
```

### Cannot find Java compiler
```bash
# Set JAVA_HOME
export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
```

### APK won't install
```bash
# Check minimum SDK version
# In android/app/build.gradle: minSdkVersion 29

# Uninstall first
adb uninstall com.pxs3c

# Then install
adb install app-debug.apk
```

## Running on Device

1. Launch PXS3C app
2. Tap "Load Game"
3. Browse to PS3 game files (.self or .elf)
4. Select and run

**Expected Performance:**
- Real Steel: 50-60 FPS (with LLVM JIT)
- Wipeout HD: 40-60 FPS
- Gran Turismo 6: 30-50 FPS

## Build Configuration

Edit `android/app/build.gradle` to customize:

```gradle
android {
    defaultConfig {
        minSdkVersion 29      // Minimum Android 10
        targetSdkVersion 34   // Target Android 14
        versionCode 1
        versionName "0.1"
        
        ndk {
            abiFilters 'arm64-v8a'  // ARM64 only
        }
    }
}
```

## Signing for Release

Generate signing key:
```bash
keytool -genkey -v -keystore ~/pxs3c-release-key.keystore \
  -keyalg RSA -keysize 2048 -validity 10000 -alias pxs3c
```

Configure in `android/app/build.gradle`:
```gradle
signingConfigs {
    release {
        storeFile file("/path/to/pxs3c-release-key.keystore")
        storePassword "your-store-password"
        keyAlias "pxs3c"
        keyPassword "your-key-password"
    }
}

buildTypes {
    release {
        signingConfig signingConfigs.release
    }
}
```

## Support

For issues:
- Check device storage has 500MB+ free space
- Verify game files are valid PS3 executables
- Enable VSync in settings for smooth playback
- Try reducing frame rate to 30 FPS if stuttering

---

**Happy emulating! 🚀**

*For x86-64 desktop builds, see the main README.md*

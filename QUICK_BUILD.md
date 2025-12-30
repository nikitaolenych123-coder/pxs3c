# Швидке Білденння APK

## GitHub Actions (Рекомендовано)

APK автоматично будується при кожному push:

1. **Push на GitHub**
   ```bash
   git push origin main
   ```

2. **Перейди на GitHub Actions**
   - https://github.com/nikitaolenych123-coder/pxs3c/actions

3. **Скачай APK з Artifacts**
   - Android Build → Artifacts
   - app-debug (або app-release)

4. **Встанови на телефон**
   ```bash
   adb install -r app-debug.apk
   ```

## Локально (з Android SDK)

Якщо у тебе є `ANDROID_HOME` налаштований:

```bash
cd android
gradle assembleDebug

# APK буде тут:
# app/build/outputs/apk/debug/app-debug.apk
```

## Docker (Якщо обоє вищенаведене недоступне)

```bash
docker build -f Dockerfile.android -t pxs3c-build .
docker run -v $(pwd):/workspace pxs3c-build
# APK у android/app/build/outputs/apk/debug/
```

## Встановлення

```bash
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

---

**Найпростіше**: Просто push на GitHub і скачай з Actions! 🚀

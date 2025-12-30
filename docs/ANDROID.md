# Android (NDK/Vulkan) план

Ціль: Android 10+ (API 29+), Snapdragon 8xx, Adreno 735 (актуальний драйвер Qualcomm).

## Мінімальні вимоги
- Vulkan 1.1+
- 4GB RAM (режим економії памʼяті)
- NEON/ARM64 (AArch64)

## Кроки
1. Створити Gradle-проєкт з NDK (CMake toolchain)
2. Інтегрувати `pxs3c_core` як native lib
3. Додати Java/Kotlin UI-оболонку, SurfaceView/TextureView для відображення
4. Vulkan init через `ANativeWindow` і `vkCreateAndroidSurfaceKHR`
5. Додати shader cache на диску (app storage)
6. Профайлінг і налаштування для Adreno (pipeline barriers, subpass)
7. Адаптивний фрейм-пейсінг: `FramePacer` 60/30 FPS з fallback за середнім часом кадру

## Оптимізації (Adreno 735)
- Зменшення алокацій, фіксований пул дескрипторів
- Режим низької латентності: подвійний буфер + відкладена синхронізація
- Презент: `FIFO` (Android), уникати енерговитратних режимів
- Розглянути `VK_QCOM_render_pass_transform` для апаратної орієнтації
- Обмежити барʼєри й синхронізацію до необхідного мінімуму
- Якщо середній час кадру > ~18мс на цілі 60FPS, падати на 30FPS; якщо < ~20мс на цілі 30FPS — пробувати 60FPS

## Система налаштувань

### Реалізовані налаштування
1. **Цільовий FPS**: 30 або 60 FPS з адаптивним перемиканням
   - UI: SeekBar у SettingsActivity (30/60)
   - JNI: `nativeSetTargetFps(fps)`
   - Native: `Emulator::setTargetFps()` → `FramePacer::setTargetFps()`

2. **VSync (Present Mode)**: FIFO/MAILBOX/IMMEDIATE
   - UI: Switch у SettingsActivity (On/Off)
   - JNI: `nativeSetVsync(enabled)`
   - Native: `Emulator::setVsync()` → `VulkanRenderer::setPresentModeAndroid()`
   - FIFO (VSync On): гарантована синхронізація з оновленням екрану, без тірингу
   - MAILBOX/IMMEDIATE (VSync Off): знижена латентність, можлива нестабільність FPS

3. **Clear Color (Колір фону)**: RGB компоненти (0.0-1.0)
   - UI: EditText поля для R, G, B у SettingsActivity
   - JNI: `nativeSetClearColor(r, g, b)`
   - Native: `Emulator::setClearColor()` → `VulkanRenderer::setClearColor()`
   - Використовується у `drawFrame()` для очищення swapchain буфера

### Потік даних
```
SettingsActivity (Apply button)
  ↓ Intent extras
MainActivity.onActivityResult()
  ↓ JNI calls
pxs3c_jni.cpp (native methods)
  ↓
Emulator (setClearColor/setVsync/setTargetFps)
  ↓
VulkanRenderer/FramePacer (apply settings)
```

### Файли
- `android/app/src/main/java/com/pxs3c/SettingsActivity.kt`
- `android/app/src/main/res/layout/activity_settings.xml`
- `src/android/jni/pxs3c_jni.cpp` (JNI bindings)
- `src/core/Emulator.{h,cpp}` (wrapper methods)
- `src/rsx/VulkanRenderer.{h,cpp}` (rendering settings)
- `src/core/FramePacer.{h,cpp}` (FPS management)

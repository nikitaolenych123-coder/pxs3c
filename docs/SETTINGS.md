# Система налаштувань

Реалізована система налаштувань із runtime-конфігурацією емулятора, подібною до aps3e.

## Архітектура

```
User → SettingsActivity → MainActivity → JNI → Emulator → VulkanRenderer/FramePacer
```

### Компоненти

1. **SettingsActivity** (`android/app/src/main/java/com/pxs3c/SettingsActivity.kt`)
   - UI: SeekBar (FPS), Switch (VSync), EditText (RGB)
   - Apply button: відправляє дані назад у MainActivity через Intent

2. **MainActivity** (`android/app/src/main/java/com/pxs3c/MainActivity.kt`)
   - onActivityResult: обробляє дані від SettingsActivity
   - Викликає JNI методи для застосування налаштувань

3. **JNI Bridge** (`src/android/jni/pxs3c_jni.cpp`)
   - `nativeSetTargetFps(fps)`: змінює цільовий FPS
   - `nativeSetVsync(enabled)`: перемикає VSync (FIFO/MAILBOX)
   - `nativeSetClearColor(r, g, b)`: встановлює колір фону

4. **Emulator** (`src/core/Emulator.{h,cpp}`)
   - Wrapper-методи для налаштувань
   - Делегує виклики до VulkanRenderer і FramePacer

5. **VulkanRenderer** (`src/rsx/VulkanRenderer.{h,cpp}`)
   - `setClearColor()`: змінює clearR_/G_/B_ для drawFrame()
   - `setPresentModeAndroid()`: пересоздає swapchain з новим VkPresentMode

6. **FramePacer** (`src/core/FramePacer.{h,cpp}`)
   - `setTargetFps()`: змінює targetFps_ для адаптивного темпу

## Налаштування

### 1. Цільовий FPS
- **Опції**: 30 або 60 FPS
- **UI**: SeekBar у SettingsActivity
- **Поведінка**: 
  - 60 FPS: оптимальний режим для сучасних ігор
  - 30 FPS: економний режим для важких ігор або слабких пристроїв
  - Адаптивне перемикання: якщо середній час кадру > ~18мс на 60FPS → падає до 30FPS; якщо < ~20мс на 30FPS → підіймає до 60FPS

### 2. VSync (Present Mode)
- **Опції**: On (FIFO) / Off (MAILBOX або IMMEDIATE)
- **UI**: Switch у SettingsActivity
- **Поведінка**:
  - **FIFO (VSync On)**: синхронізація з оновленням екрану, без тірингу, стабільний FPS
  - **MAILBOX/IMMEDIATE (VSync Off)**: низька латентність, можлива нестабільність FPS, тірінг
  - Для Adreno 735 рекомендується FIFO для балансу між стабільністю та енергоспоживанням

### 3. Clear Color (Колір фону)
- **Опції**: RGB компоненти (0.0-1.0)
- **UI**: Три EditText поля (R, G, B) у SettingsActivity
- **Поведінка**: змінює колір, яким очищається framebuffer у `VulkanRenderer::drawFrame()`
- **Приклад**:
  - Чорний: R=0.0, G=0.0, B=0.0
  - Синій: R=0.0, G=0.0, B=1.0
  - Червоний: R=1.0, G=0.0, B=0.0

## Використання

### З коду (JNI)
```cpp
// Встановити FPS
Java_com_pxs3c_MainActivity_nativeSetTargetFps(env, obj, 60);

// Увімкнути VSync
Java_com_pxs3c_MainActivity_nativeSetVsync(env, obj, true);

// Встановити синій колір фону
Java_com_pxs3c_MainActivity_nativeSetClearColor(env, obj, 0.0f, 0.0f, 1.0f);
```

### З Kotlin (MainActivity)
```kotlin
override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
    if (requestCode == SETTINGS_REQUEST && resultCode == RESULT_OK && data != null) {
        val fps = data.getIntExtra("fps", 60)
        val vsync = data.getBooleanExtra("vsync", true)
        val clearR = data.getFloatExtra("clearR", 0.0f)
        val clearG = data.getFloatExtra("clearG", 0.0f)
        val clearB = data.getFloatExtra("clearB", 0.0f)
        
        nativeSetTargetFps(fps)
        nativeSetVsync(vsync)
        nativeSetClearColor(clearR, clearG, clearB)
    }
}
```

## Розширення

Для додавання нових налаштувань:

1. Додайте поле у `Config.h` (якщо потрібне постійне зберігання)
2. Додайте UI елемент у `activity_settings.xml`
3. Обробіть у `SettingsActivity.kt` (читання/запис)
4. Додайте JNI метод у `pxs3c_jni.cpp`
5. Додайте wrapper у `Emulator.h/.cpp`
6. Реалізуйте логіку у відповідному компоненті (VulkanRenderer, FramePacer, тощо)

## Майбутні розширення

- Масштабування роздільної здатності (resolutionScale)
- Аудіо налаштування (громкість, латентність)
- Керування (віртуальні кнопки, геймпад)
- Дебаг режим (FPS counter, GPU stats)
- Збереження налаштувань у SharedPreferences

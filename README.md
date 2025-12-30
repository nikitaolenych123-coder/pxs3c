Експериментальний мобільний емулятор PS3 для Android 10+ з Snapdragon 855+ (Adreno 735), 4GB+ RAM.

**Статус**: ранній прототип з повною інфраструктурою білду, Vulkan-рендером для Adreno, адаптивним фрейм-пейсингом (60/30 FPS), системою налаштувань, мостом для RPCS3, **ELF loader та базовим PPU інтерпретатором**.

**Особливості**:
- ✅ Android NDK з Vulkan 1.1+ для Adreno 735
- ✅ Адаптивний FPS: автоперемикання між 60↔30 FPS залежно від навантаження
- ✅ Система налаштувань: FPS, VSync, колір фону (як у aps3e)
- ✅ Міст для RPCS3: динамічне завантаження без порушення ліцензії
- ✅ ELF Loader: підтримка PS3 executables (PowerPC 64-bit big-endian)
- ✅ Memory Manager: 256MB RAM з protection та endian conversion
- ✅ PPU Interpreter: базова емуляція PowerPC 970 (~20 інструкцій)
- ✅ Готова структура: PPU/SPU/RSX інтерфейси, JNI, UI

Проєкт створює чисту архітектуру для інтеграції компонентів з відкритих проєктів (RPCS3, aps3e) за умови сумісних ліцензій.

## Збірка

### Linux (тестування)
```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/pxs3c_smoke
```

### Android (NDK)
```bash
cd android
./gradlew assembleDebug
# APK: android/app/build/outputs/apk/debug/app-debug.apk
```

Або через Android Studio: відкрийте `android/` як проєкт.

## Архітектура

- **Ядро**: C++17, CMake, кросплатформенно
- **ELF Loader**: PS3 ELF64 big-endian PowerPC з автоматичним mapping
- **Memory Manager**: 256MB RAM з R/W/X protection та endian conversion
- **PPU**: PowerPC 970 інтерпретатор з ~20 базовими інструкціями
- **RSX**: Vulkan 1.1+ бекенд для Adreno 735 з FIFO/MAILBOX/IMMEDIATE режимами
- **FramePacer**: адаптивне перемикання 60↔30 FPS з EMA-згладжуванням
- **UI**: Android SurfaceView + SettingsActivity (FPS, VSync, Clear Color)
- **JNI**: нативні методи для емулятора та налаштувань

Деталі: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/ANDROID.md](docs/ANDROID.md), [docs/CPU_EMULATION.md](docs/CPU_EMULATION.md), [docs/INTEGRATION.md](docs/INTEGRATION.md)

## Ліцензії

Перш ніж інтегрувати код з інших проєктів (наприклад, RPCS3), обов’язково перевіряйте їхні ліцензії та сумісність. Нотатки: [docs/LICENSING.md](docs/LICENSING.md).

## Налаштування (SettingsActivity)

1. **FPS**: 30 або 60 (SeekBar)
2. **VSync**: On/Off (Switch) — контролює VkPresentMode (FIFO/MAILBOX)
3. **Clear Color**: RGB компоненти 0.0-1.0 (EditText) — колір фону рендеру

Налаштування застосовуються динамічно через JNI без перезапуску емулятора.

## Інтеграція RPCS3 (без копіювання коду)

- Міст для динамічного завантаження: [Rpcs3Bridge](src/cpu/engines/Rpcs3Bridge.h)
- Інструкції: [docs/INTEGRATION.md](docs/INTEGRATION.md)
- Сабмодулі: [external/README.md](external/README.md)

**Важливо**: RPCS3 використовує GPLv2 — інтеграція можлива лише через динамічне завантаження окремої бібліотеки.

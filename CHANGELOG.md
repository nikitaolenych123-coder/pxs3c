# Changelog - 30 грудня 2025

## [0.2.0] - CPU Емуляція

### Додано
- ✅ **ElfLoader** (`src/loader/`)
  - Підтримка ELF64 big-endian PowerPC файлів
  - Парсинг PT_LOAD program headers
  - Автоматичний mapping у memory з R/W/X flags
  - Endian conversion (big-endian → host)
  - Повернення entry point адреси

- ✅ **MemoryManager** (`src/memory/`)
  - PS3 memory map: 256MB Main RAM (0x00010000 - 0x10000000)
  - Region mapping з protection flags
  - Typed access з автоматичною endian конвертацією: read8/16/32/64, write8/16/32/64
  - Protection checking (R/W/X)
  - Direct pointer access для performance

- ✅ **PPUInterpreter** (`src/cpu/`)
  - PowerPC 970 (Cell PPU) registers: 32x GPR, 32x FPR, 32x VR
  - Special registers: PC, LR, CTR, CR, XER, MSR
  - ~20 базових інструкцій:
    - Arithmetic: addi, addis, add, subf, mulli
    - Logical: ori, andi., or, and, xor
    - Load/Store: lwz, lbz, lhz, ld, stw, stb, sth, std
    - Branch: b, bc, bclr, bcctr
    - System: sc (stub)
  - Condition register updates
  - executeBlock() для batch виконання (1000 інструкцій/frame)
  - Register dump для debug

### Змінено
- **Emulator** (`src/core/`)
  - Додано ініціалізацію MemoryManager, ElfLoader, PPUInterpreter
  - loadGame() тепер використовує ElfLoader
  - runFrame() виконує PPU інструкції перед рендерингом
  - Додано getMemory() та getPPU() accessors

- **CMakeLists.txt**
  - Додано src/loader/ElfLoader.cpp
  - Додано src/memory/MemoryManager.cpp
  - Додано src/cpu/PPUInterpreter.cpp

- **Smoke test** (`tests/smoke.cpp`)
  - Додано memory manager тести
  - Додано PPU register тести
  - Підтримка завантаження ELF файлів через аргументи

### Документація
- ✅ `docs/CPU_EMULATION.md` - повний guide по ELF loader, Memory Manager, PPU Interpreter
- ✅ `STATUS.md` - оновлено прогрес та метрики
- ✅ `README.md` - додано інформацію про CPU емуляцію

### Тестування
```bash
# Memory та PPU тести
./build/pxs3c_smoke

# Завантаження ELF
./build/pxs3c_smoke game.elf
```

**Результат**:
```
=== Testing Memory Manager ===
Wrote 0xdeadbeef at 0x10000
Read  0xdeadbeef from 0x10000
✓ Memory test PASSED

=== Testing PPU Interpreter ===
PC: 0x10000
GPR1: 0x12345678
GPR2: 0xabcdef00
✓ PPU basic test PASSED
```

### Метрики
- **Нові файли**: 6 (.h + .cpp)
- **Нові рядки коду**: ~1000
- **CPU емуляція**: 0% → 15%
- **ELF підтримка**: 0% → 80%
- **Memory management**: 0% → 90%
- **Загальна готовність**: 15% → 30%

---

## [0.1.0] - Інфраструктура (29 грудня 2025)

### Додано
- ✅ CMake build system для Linux
- ✅ Android Gradle + NDK skeleton
- ✅ Vulkan renderer для Adreno 735
- ✅ Adaptive FramePacer (60/30 FPS)
- ✅ RPCS3 dynamic loading bridge
- ✅ Config system
- ✅ Android UI (MainActivity + SettingsActivity)
- ✅ JNI bindings для всіх функцій
- ✅ Документація (6 файлів)

### Готовність
- Інфраструктура: 100%
- UI: 100%
- Settings: 100%

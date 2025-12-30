# ELF Loader та CPU емуляція

## ELF Loader

### Огляд
ElfLoader підтримує завантаження 64-bit big-endian PowerPC ELF файлів (PS3 executables).

### Підтримувані формати
- **ELF64**: 64-bit ELF files
- **Big-endian**: PowerPC 64-bit (EM_PPC64 = 21)
- **Program headers**: PT_LOAD, PT_DYNAMIC, PT_TLS
- **PS3-specific**: PT_SCE_* headers (RELA, PROCPARAM, MODULE_PARAM)

### Використання

```cpp
#include "loader/ElfLoader.h"
#include "memory/MemoryManager.h"

MemoryManager memory;
memory.init();

ElfLoader loader;
if (loader.load("game.elf", &memory)) {
    uint64_t entry = loader.getEntryPoint();
    // Set PPU PC to entry point
}
```

### Процес завантаження

1. **Валідація header**: перевірка ELF magic, class (64-bit), endianness (big), machine (PPC64)
2. **Парсинг program headers**: читання PT_LOAD сегментів
3. **Mapping memory**: створення memory regions з відповідними flags (R/W/X)
4. **Копіювання даних**: завантаження file data у memory
5. **Повернення entry point**: адреса першої інструкції

### Структура

```cpp
struct LoadedSegment {
    uint64_t vaddr;           // Virtual address
    uint64_t size;            // Segment size
    uint32_t flags;           // Protection flags (R/W/X)
    std::vector<uint8_t> data; // Segment data
};
```

### Endian conversion
Всі multi-byte значення конвертуються з big-endian (PS3) у host endianness:
- `swapEndian16/32/64()` для автоматичної конвертації

## Memory Manager

### PS3 Memory Map (спрощена)

```
0x00010000 - 0x10000000: Main RAM (256MB)
0x20000000 - 0x30000000: User space (256MB)
0xC0000000 - 0xD0000000: RSX memory (256MB)
0xD0000000 - 0xE0000000: MMIO
```

### API

#### Ініціалізація
```cpp
MemoryManager memory;
memory.init(); // Pre-allocates main RAM
```

#### Memory mapping
```cpp
// Map region with protection flags
memory.mapRegion(vaddr, size, MEM_PROT_READ | MEM_PROT_WRITE);

// Get region info
MemoryRegion* region = memory.getRegion(vaddr);
```

#### Typed access (з endian swap)
```cpp
uint8_t  val8  = memory.read8(addr);
uint16_t val16 = memory.read16(addr);  // Big-endian → host
uint32_t val32 = memory.read32(addr);
uint64_t val64 = memory.read64(addr);

memory.write32(addr, 0xDEADBEEF); // Host → big-endian
```

#### Raw access
```cpp
// Read/write arbitrary data
memory.read(vaddr, buffer, size);
memory.write(vaddr, buffer, size);

// Direct pointer (небезпечно, але швидко)
uint8_t* ptr = memory.getPointer(vaddr);
```

### Protection flags
```cpp
MEM_PROT_READ  = 0x4  // Readable
MEM_PROT_WRITE = 0x2  // Writable
MEM_PROT_EXEC  = 0x1  // Executable
```

## PPU Interpreter

### Огляд
PPUInterpreter - базовий інтерпретатор PowerPC 970 (Cell PPU) інструкцій.

### Регістри

```cpp
struct PPURegisters {
    std::array<uint64_t, 32> gpr;  // General Purpose Registers
    std::array<double, 32> fpr;     // Floating Point Registers
    std::array<uint128_t, 32> vr;   // Vector Registers (Altivec)
    
    uint64_t pc;   // Program Counter
    uint64_t lr;   // Link Register
    uint64_t ctr;  // Count Register
    uint32_t cr;   // Condition Register
    uint32_t xer;  // Fixed-Point Exception Register
    // ...
};
```

### Використання

```cpp
PPUInterpreter ppu;
ppu.init(memory);
ppu.setPC(entryPoint);

// Execute one instruction
ppu.executeInstruction();

// Execute block (1000 instructions)
ppu.executeBlock(1000);

// Access registers
uint64_t r1 = ppu.getGPR(1);
ppu.setGPR(1, value);
```

### Підтримувані інструкції

#### Arithmetic/Logical
- `addi`, `addis`, `add`, `subf`
- `ori`, `andi.`, `or`, `and`, `xor`
- `mulli`, `cmpli`, `cmpi`

#### Load/Store
- `lwz`, `lbz`, `lhz`, `ld`
- `stw`, `stb`, `sth`, `std`
- `lwzu`, `lbzu`, `ldu`, `stdu` (with update)

#### Branch
- `b` - unconditional branch
- `bc` - conditional branch
- `bclr` - branch to link register
- `bcctr` - branch to count register

#### System
- `sc` - system call (stub, logs only)

#### Floating Point
- Stub (not implemented yet)

### Виконання

```cpp
void PPUInterpreter::decodeAndExecute(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 5);
    
    switch (opcode) {
        case 14: // addi
            executeArithmetic(instr);
            break;
        case 18: // b
            executeBranch(instr);
            break;
        case 32: // lwz
            executeLoadStore(instr);
            break;
        // ...
    }
}
```

### Condition Register
CR оновлюється для інструкцій з `.` суфіксом:
- `LT` (0x8): result < 0
- `GT` (0x4): result > 0
- `EQ` (0x2): result == 0
- `SO` (0x1): summary overflow

### Branch Conditions
```cpp
bool checkCondition(uint32_t bo, uint32_t bi) {
    // bo: branch options
    // bi: condition register bit
    bool ctr_ok = (bo & 0x04) || ((--ctr != 0) ^ ((bo & 0x02) != 0));
    bool cond_ok = (bo & 0x10) || (((cr >> (31 - bi)) & 1) == ((bo >> 3) & 1));
    return ctr_ok && cond_ok;
}
```

## Інтеграція з Emulator

```cpp
class Emulator {
    std::unique_ptr<MemoryManager> memory_;
    std::unique_ptr<ElfLoader> elfLoader_;
    std::unique_ptr<PPUInterpreter> ppu_;
    
    bool loadGame(const char* path) {
        if (elfLoader_->load(path, memory_.get())) {
            ppu_->setPC(elfLoader_->getEntryPoint());
            return true;
        }
        return false;
    }
    
    void runFrame() {
        ppu_->executeBlock(1000); // ~1000 instructions per frame
        renderer_->drawFrame();
    }
};
```

## Тестування

```bash
./pxs3c_smoke              # Memory/PPU tests
./pxs3c_smoke game.elf     # Load and test ELF
```

### Приклад виводу
```
=== PXS3C Emulator Test ===
Initializing PS3 memory map...
Main RAM: 0x10000 - 0x10010000
Memory Regions (1):
  0x10000 - 0x10010000 (256 MB) flags=0x6

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

## Обмеження та TODO

### ElfLoader
- ⏳ SELF (signed executables) не підтримується
- ⏳ Dynamic linking/relocations базовий
- ⏳ TLS (Thread-Local Storage) не реалізовано

### MemoryManager
- ⏳ Відсутня пагінація
- ⏳ RSX/MMIO regions не mapped за замовчуванням
- ⏳ Немає захисту від одночасного доступу (thread-safety)

### PPUInterpreter
- ⏳ Тільки ~20 базових інструкцій
- ⏳ Floating point не реалізовано
- ⏳ Vector/Altivec не реалізовано
- ⏳ Відсутній JIT компілятор
- ⏳ Syscalls заглушені
- ⏳ Exceptions не обробляються

### Продуктивність
- Інтерпретатор: ~1-5 MIPS (дуже повільно)
- Потрібен JIT: цільова продуктивність ~100-500 MIPS
- Оптимізація: block compilation, register caching

## Наступні кроки

1. **SPU інтерпретатор**: 6x SPU cores для паралельних обчислень
2. **PPU JIT**: LLVM або custom JIT для швидкості
3. **Syscall handler**: LV1/LV2 hypervisor calls
4. **RSX integration**: графічні команди → Vulkan
5. **Thread support**: багатопотоковість PPU/SPU

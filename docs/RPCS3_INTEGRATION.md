# RPCS3 ARM64 Integration Plan

## Project Goal
Create an aPS3e-style UI with full RPCS3 codebase underneath, optimized for ARM64.

## RPCS3 Core Components to Port

### 1. CPU Emulation
- [ ] **PPU (PowerPC 64-bit)**
  - RPCS3 PPU interpreter (all 700+ instructions)
  - RPCS3 PPU recompiler (LLVM → ARM64)
  - Register mapping optimization
  - Branch prediction

- [ ] **SPU (Synergistic Processing Unit)**
  - RPCS3 SPU interpreter (256 instructions)
  - RPCS3 ASMJIT recompiler
  - ARM64 NEON/SVE2 vectorization
  - 6 SPU threads scheduling

### 2. System Calls (LV2)
- [ ] Process management (sys_process_*)
- [ ] Thread management (sys_ppu_thread_*)
- [ ] Memory management (sys_memory_*)
- [ ] File system (sys_fs_*)
- [ ] Time management (sys_time_*)
- [ ] Synchronization (sys_mutex_*, sys_cond_*, sys_semaphore_*)
- [ ] ~300 total syscalls from RPCS3

### 3. RSX Graphics
- [ ] RPCS3 RSX command buffer processing
- [ ] Shader recompiler (GLSL → SPIR-V)
- [ ] Vulkan backend (from RPCS3)
- [ ] Texture cache
- [ ] Render target management

### 4. Audio (SPU Audio)
- [ ] Cellspurs audio processing
- [ ] OpenSL ES backend
- [ ] Audio resampling

### 5. File Systems
- [ ] PS3 virtual file system (/dev_hdd0, /dev_bdvd, etc.)
- [ ] PKG decryption (from RPCS3)
- [ ] SELF/SPRX decryption
- [ ] Save data management

## ARM64 Optimizations

### CPU
- Use ARMv9 SVE2 for SPU vector operations (4x128-bit → 512-bit)
- PPU register mapping to ARM64 registers
- Optimized memory barriers for ARM

### GPU
- Adreno-specific Vulkan optimizations
- Tile-based rendering
- Async compute for SPU audio

### Memory
- Direct memory mapping where possible
- LPDDR5X bandwidth optimization

## UI Design (aPS3e Style)

### Main Screen
```
┌─────────────────────────────────────┐
│  [≡]  PXS3C         [⚙️] [ℹ️]       │
├─────────────────────────────────────┤
│                                     │
│  ┌──────┐  ┌──────┐  ┌──────┐     │
│  │      │  │      │  │      │     │
│  │ Game │  │ Game │  │ Game │     │
│  │  #1  │  │  #2  │  │  #3  │     │
│  │      │  │      │  │      │     │
│  └──────┘  └──────┘  └──────┘     │
│                                     │
│         [+ Add Game]                │
│                                     │
└─────────────────────────────────────┘
```

### Settings Tabs
1. **System** - Firmware, language, time zone
2. **CPU** - PPU decoder, SPU decoder, threads
3. **GPU** - Resolution, renderer, VSync, shader mode
4. **Audio** - Backend, volume, latency
5. **Input** - Controller mapping
6. **Network** - PSN settings
7. **Advanced** - Debug options, patches

## File Structure
```
src/
├── rpcs3/               # RPCS3 core code (adapted)
│   ├── Emu/
│   │   ├── Cell/       # PPU/SPU emulation
│   │   ├── RSX/        # Graphics
│   │   ├── Memory/     # Memory management
│   │   └── System/     # System calls
│   └── util/           # RPCS3 utilities
├── android/
│   ├── ui/             # aPS3e-style UI
│   │   ├── GameListActivity
│   │   ├── GameSettingsActivity
│   │   └── EmulationActivity
│   └── jni/            # JNI bridge
└── patches/            # Game-specific patches
```

## Development Phases

### Phase 1: Core Foundation ✅
- [x] Basic project structure
- [x] Android build system
- [x] Vulkan initialization
- [x] Basic UI

### Phase 2: RPCS3 CPU Integration (Current)
- [ ] Import RPCS3 PPU interpreter
- [ ] Import RPCS3 SPU interpreter
- [ ] ARM64 recompiler stubs
- [ ] Basic syscall handler

### Phase 3: Graphics Pipeline
- [ ] RPCS3 RSX command processing
- [ ] Shader translation
- [ ] Vulkan renderer

### Phase 4: Full System
- [ ] Complete syscall implementation
- [ ] File system virtualization
- [ ] Save data support
- [ ] PKG installation

### Phase 5: Optimization
- [ ] SVE2 SPU vectorization
- [ ] PPU ARM64 recompiler
- [ ] Profile-guided optimization
- [ ] Per-game patches

## Performance Targets

| Device | CPU | Target |
|--------|-----|--------|
| Snapdragon 8 Gen 3 | Cortex-X4 | 60 FPS (2D games) |
| Snapdragon 8 Gen 2 | Cortex-X3 | 45 FPS (2D games) |
| Snapdragon 8 Gen 1 | Cortex-X2 | 30 FPS (2D games) |

## RPCS3 Code Attribution
This project uses modified RPCS3 code:
- Original: https://github.com/RPCS3/rpcs3
- License: GNU GPL v2
- Authors: Nekotekina, kd-11, and contributors

All modifications are documented and GPL-compliant.

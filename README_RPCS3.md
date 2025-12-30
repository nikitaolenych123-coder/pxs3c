# PXS3C - RPCS3 ARM64 Port

## Overview
PXS3C is a PS3 emulator for Android ARM64 based on RPCS3 architecture.

## Architecture

### CPU Emulation (Cell Broadband Engine)
- **PPU (PowerPC Processing Unit)**: 
  - Interpreter mode for compatibility
  - LLVM JIT compiler for 60 FPS performance (desktop)
  - ARM64 recompiler for mobile (planned)
  
- **SPU (Synergistic Processing Unit)**:
  - 6 SPU threads simulation
  - SVE2 vectorization for ARM64
  - ASMJIT recompiler for SPU code

### GPU Emulation (RSX Reality Synthesizer)
- Vulkan 1.3 backend
- Shader translation (GLSL → SPIR-V)
- Tile-based rendering optimization for mobile GPUs
- Async compute for lighting/effects

### Memory System
- PS3 memory map emulation (256MB XDR + 256MB GDDR3)
- Direct memory mapping on ARM64
- Zero-copy techniques for performance

## Current Implementation Status

### ✅ Implemented
- [x] Basic PPU interpreter (724 instructions)
- [x] SPU manager (6 threads)
- [x] Memory manager with PS3 layout
- [x] ELF/SELF loader
- [x] Vulkan renderer initialization
- [x] Android UI (MainActivity, SettingsActivity, AdvancedSettingsActivity)
- [x] Frame pacer (30/60 FPS)
- [x] File picker (PKG/ISO/ELF support)

### 🚧 In Progress
- [ ] SPU recompiler with SVE2
- [ ] PPU JIT for ARM64
- [ ] RSX command processing
- [ ] Shader recompilation
- [ ] Audio backend (OpenSL ES)

### 📋 Planned
- [ ] Full syscall implementation (~300 syscalls)
- [ ] PKG decryption
- [ ] Game patching system
- [ ] Save data management
- [ ] Trophy support
- [ ] Network emulation (PSN)

## Performance Targets

### Snapdragon 8s Gen 3 (Poco F6)
- **CPU**: Cortex-X4 (3.0 GHz) + A720 + A520
- **GPU**: Adreno 735
- **RAM**: LPDDR5X

**Target Performance**:
- Simple games (2D, indie): 60 FPS
- Medium games (PS3 early titles): 30-45 FPS  
- Heavy games (GoW3, TLOU): 20-30 FPS

### Optimization Techniques
1. **ARMv9 SVE2**: 4x faster SPU vectorization
2. **Vulkan 1.3**: Dynamic rendering, GPL pre-caching
3. **Async Compute**: Parallel GPU workloads
4. **LTO + PGO**: Link-time + profile-guided optimization
5. **Frame Skip**: Dynamic frame dropping for stability

## Building

### Prerequisites
- Android NDK r27+
- CMake 3.22+
- Vulkan SDK

### Compile
```bash
cd android
./gradlew assembleRelease
```

### Optimization Flags
```cmake
-O3 -Ofast -march=armv9-a+sve2 -flto=full -ffast-math
```

## RPCS3 Compatibility
This project aims for RPCS3 compatibility at the API level:
- Same syscall numbers
- Same memory layout
- Same file formats (PKG, SELF, EDAT)
- Save data compatibility

## Credits
Based on RPCS3 architecture by Nekotekina, kd-11, and contributors.

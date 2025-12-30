#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <memory>

namespace pxs3c {

class MemoryManager;
class SyscallHandler;
class PPUInterpreter;
class LLVMJITCompiler;

// Forward declare uint128_t (defined in PPUInterpreter.h)
union uint128_t;

// PPU JIT compiler - translates PowerPC blocks to native code via LLVM
typedef uint64_t (*PPUCompiledBlock)(uint64_t* gpr, double* fpr, uint128_t* vr,
                                      uint64_t pc, uint64_t lr, uint32_t cr);

struct JITBlockHeader {
    uint64_t startPC;
    uint64_t blockSize;
    uint32_t instructionCount;
    PPUCompiledBlock compiled;
    uint64_t callCount;  // How many times executed
    uint64_t compiledAt;  // When compiled
};

// JIT compilation cache with LLVM backend
class PPUJIT {
public:
    PPUJIT();
    ~PPUJIT();
    
    bool init(PPUInterpreter* ppu, MemoryManager* memory);
    void shutdown();
    
    // Compile a block starting at PC
    bool compileBlock(uint64_t pc, uint32_t maxInstructions = 100);
    
    // Try to execute compiled block, fallback to interpreter if not found
    bool executeBlock(uint64_t& pc, uint64_t maxInstructions);
    
    // Clear cache
    void clearCache();
    
    // Statistics
    uint64_t getCacheSize() const { return cache_.size(); }
    uint64_t getTotalCompilations() const { return totalCompilations_; }
    
private:
    PPUInterpreter* ppu_;
    MemoryManager* memory_;
    std::unique_ptr<LLVMJITCompiler> llvmJit_;
    std::map<uint64_t, std::unique_ptr<JITBlockHeader>> cache_;
    uint64_t totalCompilations_;
    uint64_t cacheHits_;
    uint64_t cacheMisses_;
};

} // namespace pxs3c

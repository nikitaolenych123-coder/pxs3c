#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <memory>

namespace pxs3c {

class MemoryManager;
class SyscallHandler;
class PPUInterpreter;

// Simple uint128 helper
union uint128 {
    uint64_t u64[2];
    uint32_t u32[4];
    
    uint128() { u64[0] = 0; u64[1] = 0; }
};

// Simple PPU JIT compiler - translates PowerPC blocks to x86-64
// For now, uses inline caching and basic optimization

typedef uint64_t (*PPUCompiledBlock)(uint64_t* gpr, double* fpr, uint128* vr,
                                      uint64_t pc, void* memory);

struct JITBlockHeader {
    uint64_t startPC;
    uint64_t blockSize;
    uint32_t instructionCount;
    PPUCompiledBlock compiled;
    uint64_t callCount;  // How many times executed
    uint64_t compiledAt;  // When compiled
};

// JIT compilation cache
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
    std::map<uint64_t, std::unique_ptr<JITBlockHeader>> cache_;
    uint64_t totalCompilations_;
    uint64_t cacheHits_;
    uint64_t cacheMisses_;
    
    // Stub: Actual JIT compilation not implemented (platform-specific)
    // Would need:
    // - x86-64 code generation
    // - Register allocation
    // - Instruction selection
    // For now, return false to use interpreter
    bool compileBlockX86(uint64_t pc, std::vector<uint32_t>& instructions,
                         JITBlockHeader& header);
};

} // namespace pxs3c

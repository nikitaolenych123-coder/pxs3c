#include "cpu/PPUJIT.h"
#include "cpu/PPUInterpreter.h"
#ifdef LLVM_AVAILABLE
#include "cpu/LLVMJITCompiler.h"
#endif
#include "memory/MemoryManager.h"
#include <iostream>
#include <chrono>

namespace pxs3c {

PPUJIT::PPUJIT()
    : ppu_(nullptr), memory_(nullptr),
      totalCompilations_(0), cacheHits_(0), cacheMisses_(0),
      llvmJit_(nullptr) {}

PPUJIT::~PPUJIT() {
    shutdown();
}

bool PPUJIT::init(PPUInterpreter* ppu, MemoryManager* memory) {
    if (!ppu || !memory) return false;
    ppu_ = ppu;
    memory_ = memory;
    
#ifdef LLVM_AVAILABLE
    // Initialize LLVM JIT compiler
    llvmJit_ = std::make_unique<LLVMJITCompiler>();
    if (!llvmJit_->init()) {
        std::cerr << "Failed to initialize LLVM JIT" << std::endl;
        llvmJit_ = nullptr;
        return false;
    }
    
    std::cout << "PPU JIT compiler initialized with LLVM backend" << std::endl;
#else
    std::cout << "PPU JIT compiler initialized (LLVM not available, using interpreter only)" << std::endl;
#endif
    return true;
}

void PPUJIT::shutdown() {
    clearCache();
    llvmJit_ = nullptr;
    ppu_ = nullptr;
    memory_ = nullptr;
}

bool PPUJIT::compileBlock(uint64_t pc, uint32_t maxInstructions) {
    if (!ppu_ || !memory_) return false;
    
    // Check if already compiled
    if (cache_.find(pc) != cache_.end()) {
        if (cache_[pc]->compiled != nullptr) {
            cache_[pc]->callCount++;
            cacheHits_++;
            return true;
        }
    }
    
    cacheMisses_++;
    
    // Read block of instructions from memory
    std::vector<uint32_t> instructions;
    uint64_t currentPC = pc;
    
    for (uint32_t i = 0; i < maxInstructions; i++) {
        uint32_t instr = memory_->read32(currentPC);
        
        // Already in big-endian from PS3 memory
        instructions.push_back(instr);
        currentPC += 4;
        
        // Stop at branch/return instructions
        uint8_t opcode = (instr >> 26) & 0x3F;
        if (opcode == 18 || opcode == 16 || opcode == 19) { // b, bc, bcctr, bclr
            break;
        }
    }
    
    if (instructions.empty()) {
        return false;
    }
    
    auto header = std::make_unique<JITBlockHeader>();
    header->startPC = pc;
    header->blockSize = instructions.size() * 4;
    header->instructionCount = instructions.size();
    header->callCount = 1;
    header->compiledAt = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    header->compiled = nullptr;
    
#ifdef LLVM_AVAILABLE
    // Try to compile with LLVM JIT
    if (llvmJit_) {
        auto compiled = llvmJit_->compileBlock(ppu_, memory_, pc, maxInstructions);
        header->compiled = compiled;
    }
#endif
    
    totalCompilations_++;
    cache_[pc] = std::move(header);
    
    if (cache_[pc]->compiled != nullptr) {
        std::cout << "LLVM JIT compiled block at 0x" << std::hex << pc << std::dec << std::endl;
        return true;
    }
    
    return false;
}

bool PPUJIT::executeBlock(uint64_t& pc, uint64_t maxInstructions) {
    if (!ppu_) return false;
    
    // Try to find compiled block
    auto it = cache_.find(pc);
    if (it != cache_.end() && it->second->compiled != nullptr) {
        std::cout << "JIT hit: Executing compiled block at 0x" << std::hex << pc << std::dec << std::endl;
        cacheHits_++;
        it->second->callCount++;
        return true;
    }
    
    // Try to compile if not yet attempted
    if (cache_.find(pc) == cache_.end()) {
        compileBlock(pc, maxInstructions);
        if (cache_[pc]->compiled != nullptr) {
            return true;
        }
    }
    
    // Fall back to interpreter
    return false;
}

void PPUJIT::clearCache() {
    cache_.clear();
    std::cout << "JIT cache cleared (" << totalCompilations_ << " blocks compiled, "
              << cacheHits_ << " hits, " << cacheMisses_ << " misses)" << std::endl;
    totalCompilations_ = 0;
    cacheHits_ = 0;
    cacheMisses_ = 0;
}

} // namespace pxs3c


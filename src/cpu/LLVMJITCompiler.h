#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/TargetSelect.h"
#include "cpu/PPUInterpreter.h"

namespace pxs3c {

class MemoryManager;
class PPUInterpreter;

// JIT compilation engine using LLVM for 60 FPS performance
class LLVMJITCompiler {
public:
    LLVMJITCompiler();
    ~LLVMJITCompiler();
    
    bool init();
    
    // Compile a PowerPC block to native x86-64 code
    typedef uint64_t (*CompiledFunc)(uint64_t* gpr, double* fpr, uint128_t* vr,
                                      uint64_t pc, uint64_t lr, uint32_t cr);
    
    CompiledFunc compileBlock(PPUInterpreter* ppu, MemoryManager* memory,
                              uint64_t startPC, uint32_t maxInstructions);
    
private:
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
    llvm::ExecutionEngine* engine_;
    
    // Build IR for a single PowerPC instruction
    bool buildInstructionIR(llvm::IRBuilder<>& builder,
                           PPUInterpreter* ppu,
                           MemoryManager* memory,
                           uint32_t instr,
                           std::vector<llvm::Value*>& gprValues,
                           std::vector<llvm::Value*>& fprValues);
};

} // namespace pxs3c

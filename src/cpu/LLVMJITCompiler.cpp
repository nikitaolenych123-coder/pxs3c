#include "cpu/LLVMJITCompiler.h"
#include "cpu/PPUInterpreter.h"
#include "memory/MemoryManager.h"
#include "llvm/IR/Verifier.h"
#include <iostream>

namespace pxs3c {

LLVMJITCompiler::LLVMJITCompiler() : engine_(nullptr) {}

LLVMJITCompiler::~LLVMJITCompiler() {
    // ExecutionEngine will be deleted with module
}

bool LLVMJITCompiler::init() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();
    
    context_ = std::make_unique<llvm::LLVMContext>();
    module_ = std::make_unique<llvm::Module>("pxs3c_jit", *context_);
    
    std::string error;
    engine_ = llvm::EngineBuilder(std::move(module_))
        .setErrorStr(&error)
        .setEngineKind(llvm::EngineKind::JIT)
        .setOptLevel(llvm::CodeGenOptLevel::Aggressive)
        .create();
    
    if (!engine_) {
        std::cerr << "Failed to create LLVM execution engine: " << error << std::endl;
        return false;
    }
    
    std::cout << "LLVM JIT compiler initialized" << std::endl;
    return true;
}

LLVMJITCompiler::CompiledFunc LLVMJITCompiler::compileBlock(
    PPUInterpreter* ppu, MemoryManager* memory,
    uint64_t startPC, uint32_t maxInstructions) {
    
    if (!ppu || !memory || !engine_) {
        return nullptr;
    }
    
    // Get LLVM types
    auto& ctx = *context_;
    auto* i64Ty = llvm::Type::getInt64Ty(ctx);
    auto* i32Ty = llvm::Type::getInt32Ty(ctx);
    auto* i8Ty = llvm::Type::getInt8Ty(ctx);
    auto* i1Ty = llvm::Type::getInt1Ty(ctx);
    auto* doubleTy = llvm::Type::getDoubleTy(ctx);
    auto* i64PtrTy = llvm::PointerType::get(i64Ty, 0);
    auto* doublePtrTy = llvm::PointerType::get(doubleTy, 0);
    
    // Create function signature:
    // uint64_t func(uint64_t* gpr, double* fpr, uint128_t* vr, uint64_t pc, uint64_t lr, uint32_t cr)
    std::vector<llvm::Type*> paramTypes = {
        i64PtrTy,    // gpr
        doublePtrTy, // fpr
        i64PtrTy,    // vr (simplified as uint64_t*)
        i64Ty,       // pc
        i64Ty,       // lr
        i32Ty        // cr
    };
    
    auto* funcType = llvm::FunctionType::get(i64Ty, paramTypes, false);
    auto* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                        "compiled_block", module_.get());
    
    // Set parameter names for clarity
    auto argIter = func->arg_begin();
    llvm::Argument* argGpr = &*argIter;
    argGpr->setName("gpr");
    llvm::Argument* argFpr = &*(++argIter);
    argFpr->setName("fpr");
    llvm::Argument* argVr = &*(++argIter);
    argVr->setName("vr");
    llvm::Argument* argPc = &*(++argIter);
    argPc->setName("pc");
    llvm::Argument* argLr = &*(++argIter);
    argLr->setName("lr");
    llvm::Argument* argCr = &*(++argIter);
    argCr->setName("cr");
    
    // Create entry basic block
    auto* entryBB = llvm::BasicBlock::Create(ctx, "entry", func);
    llvm::IRBuilder<> builder(entryBB);
    
    // Read all instructions in block (up to branch)
    std::vector<uint32_t> instructions;
    uint64_t currentPC = startPC;
    uint32_t instrCount = 0;
    
    for (uint32_t i = 0; i < std::min(maxInstructions, 100u); i++) {
        uint32_t instr = memory->read32(currentPC);
        instructions.push_back(instr);
        currentPC += 4;
        instrCount++;
        
        // Stop at branch/conditional instructions
        uint8_t opcode = (instr >> 26) & 0x3F;
        if (opcode == 18 || opcode == 19 || opcode == 16) break; // b, bc, bcc
    }
    
    // Compile each instruction to IR
    for (uint32_t i = 0; i < instructions.size(); i++) {
        uint32_t instr = instructions[i];
        uint8_t opcode = (instr >> 26) & 0x3F;
        uint8_t ra = (instr >> 16) & 0x1F;
        uint8_t rb = (instr >> 11) & 0x1F;
        uint8_t rd = (instr >> 21) & 0x1F;
        int16_t imm = instr & 0xFFFF;
        
        // Generate optimized IR for most common instructions
        switch (opcode) {
            case 14: { // addi  rd, ra, imm
                auto* val_ra = builder.CreateLoad(i64Ty, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                auto* result = builder.CreateAdd(val_ra, 
                    llvm::ConstantInt::get(i64Ty, (int64_t)imm));
                builder.CreateStore(result, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rd)));
                break;
            }
            case 15: { // addis  rd, ra, imm
                auto* val_ra = builder.CreateLoad(i64Ty, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                auto* result = builder.CreateAdd(val_ra, 
                    llvm::ConstantInt::get(i64Ty, ((int64_t)imm) << 16));
                builder.CreateStore(result, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rd)));
                break;
            }
            case 8: { // subfic  rd, ra, imm
                auto* val_ra = builder.CreateLoad(i64Ty, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                auto* result = builder.CreateSub(
                    llvm::ConstantInt::get(i64Ty, (int64_t)imm), val_ra);
                builder.CreateStore(result, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rd)));
                break;
            }
            case 10: { // cmpli
                auto* val_ra = builder.CreateLoad(i64Ty, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                auto* cond = builder.CreateICmpULT(val_ra, 
                    llvm::ConstantInt::get(i64Ty, (uint64_t)(uint16_t)imm));
                auto* crVal = builder.CreateLoad(i32Ty, argCr);
                // Simple CR update (bits for field 0)
                auto* newCr = builder.CreateSelect(cond,
                    llvm::ConstantInt::get(i32Ty, 0x80000000),
                    llvm::ConstantInt::get(i32Ty, 0));
                builder.CreateStore(newCr, argCr);
                break;
            }
            case 28: { // andi.  rd, ra, imm
                auto* val_ra = builder.CreateLoad(i64Ty, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                auto* result = builder.CreateAnd(val_ra, 
                    llvm::ConstantInt::get(i64Ty, (uint64_t)(uint16_t)imm));
                builder.CreateStore(result, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rd)));
                break;
            }
            case 29: { // andis.  rd, ra, imm
                auto* val_ra = builder.CreateLoad(i64Ty, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                auto* result = builder.CreateAnd(val_ra, 
                    llvm::ConstantInt::get(i64Ty, ((uint64_t)(uint16_t)imm) << 16));
                builder.CreateStore(result, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rd)));
                break;
            }
            case 30: { // ori  rd, ra, imm
                auto* val_ra = builder.CreateLoad(i64Ty, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                auto* result = builder.CreateOr(val_ra, 
                    llvm::ConstantInt::get(i64Ty, (uint64_t)(uint16_t)imm));
                builder.CreateStore(result, 
                    builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rd)));
                break;
            }
            case 31: { // 2-opcode instructions (xop)
                uint16_t xop = (instr >> 1) & 0x3FF;
                if (xop == 266) { // add  rd, ra, rb
                    auto* val_ra = builder.CreateLoad(i64Ty, 
                        builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                    auto* val_rb = builder.CreateLoad(i64Ty, 
                        builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rb)));
                    auto* result = builder.CreateAdd(val_ra, val_rb);
                    builder.CreateStore(result, 
                        builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rd)));
                } else if (xop == 40) { // subf  rd, ra, rb
                    auto* val_ra = builder.CreateLoad(i64Ty, 
                        builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                    auto* val_rb = builder.CreateLoad(i64Ty, 
                        builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rb)));
                    auto* result = builder.CreateSub(val_rb, val_ra);
                    builder.CreateStore(result, 
                        builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rd)));
                } else if (xop == 444) { // or  rd, ra, rb
                    auto* val_ra = builder.CreateLoad(i64Ty, 
                        builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, ra)));
                    auto* val_rb = builder.CreateLoad(i64Ty, 
                        builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rb)));
                    auto* result = builder.CreateOr(val_ra, val_rb);
                    builder.CreateStore(result, 
                        builder.CreateGEP(i64Ty, argGpr, llvm::ConstantInt::get(i64Ty, rd)));
                }
                break;
            }
            // Other instructions: skip (stub)
            default:
                break;
        }
    }
    
    // Return next PC
    auto* nextPC = builder.CreateAdd(
        argPc, 
        llvm::ConstantInt::get(i64Ty, instrCount * 4)
    );
    builder.CreateRet(nextPC);
    
    // Verify function
    if (llvm::verifyFunction(*func, &llvm::errs())) {
        std::cerr << "Failed to verify JIT function" << std::endl;
        func->eraseFromParent();
        return nullptr;
    }
    
    // JIT compile to native code
    auto* funcPtr = engine_->getPointerToFunction(func);
    
    std::cout << "LLVM JIT compiled block at 0x" << std::hex << startPC << std::dec
              << " (" << instrCount << " instructions) -> native code" << std::endl;
    
    return (CompiledFunc)funcPtr;
}

bool LLVMJITCompiler::buildInstructionIR(
    llvm::IRBuilder<>& builder,
    PPUInterpreter* ppu,
    MemoryManager* memory,
    uint32_t instr,
    std::vector<llvm::Value*>& gprValues,
    std::vector<llvm::Value*>& fprValues) {
    
    // This would contain the actual PowerPC to LLVM IR translation
    // for each opcode. For now, return true (placeholder)
    return true;
}

} // namespace pxs3c

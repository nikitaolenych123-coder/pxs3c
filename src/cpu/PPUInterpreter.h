#pragma once

#include <cstdint>
#include <array>
#include <memory>

namespace pxs3c {

class MemoryManager;
class SyscallHandler;
class PPUJIT;

// Helper union for 128-bit vectors (defined first)
union uint128_t {
    uint64_t u64[2];
    uint32_t u32[4];
    uint16_t u16[8];
    uint8_t u8[16];
    
    uint128_t() { u64[0] = 0; u64[1] = 0; }
    uint128_t(uint64_t val) { u64[0] = val; u64[1] = 0; }
};

// PowerPC 970 (Cell PPU) General Purpose Registers
struct PPURegisters {
    std::array<uint64_t, 32> gpr;  // General Purpose Registers
    std::array<double, 32> fpr;     // Floating Point Registers
    std::array<uint128_t, 32> vr;   // Vector Registers (Altivec)
    
    uint64_t pc;                     // Program Counter
    uint64_t lr;                     // Link Register
    uint64_t ctr;                    // Count Register
    uint32_t cr;                     // Condition Register
    uint32_t xer;                    // Fixed-Point Exception Register
    uint32_t fpscr;                  // FP Status and Control Register
    uint32_t vscr;                   // Vector Status and Control Register
    
    // Special Purpose Registers
    uint64_t msr;                    // Machine State Register
    uint64_t srr0, srr1;             // Exception save/restore
    
    PPURegisters() {
        gpr.fill(0);
        fpr.fill(0.0);
        vr.fill(0);
        pc = 0; lr = 0; ctr = 0;
        cr = 0; xer = 0; fpscr = 0; vscr = 0;
        msr = 0; srr0 = 0; srr1 = 0;
    }
};

// PPU Interpreter (simplified)
class PPUInterpreter {
private:
    PPURegisters regs_;
    MemoryManager* memory_;
    SyscallHandler* syscalls_;
    bool halted_;
    std::unique_ptr<PPUJIT> jit_;  // JIT compiler for 60 FPS
    
public:
    PPUInterpreter();
    ~PPUInterpreter();

    bool init(MemoryManager* memory, SyscallHandler* syscalls = nullptr);
    void reset();
    
    // Get JIT compiler (for testing)
    PPUJIT* getJIT() const { return jit_.get(); }
    
    // Set entry point
    void setPC(uint64_t pc) { regs_.pc = pc; }
    uint64_t getPC() const { return regs_.pc; }
    
    // Execute instructions (with JIT acceleration)
    void executeInstruction();
    void executeBlock(int maxInstructions = 1000);
    
    // Register access (public for JIT)
    uint64_t getGPR(int n) const { return regs_.gpr[n]; }
    void setGPR(int n, uint64_t val) { regs_.gpr[n] = val; }
    
    // Public register arrays for JIT access
    uint64_t* gpr = nullptr;
    double* fpr = nullptr;
    uint128_t* vr = nullptr;
    
    // Debug
    void dumpRegisters() const;
    
    // Instruction decoding (public for JIT)
    void decodeAndExecute(uint32_t instruction);
    
    // Instruction groups
    void executeArithmetic(uint32_t instr);
    void executeLogical(uint32_t instr);
    void executeLoadStore(uint32_t instr);
    void executeBranch(uint32_t instr);
    void executeSystem(uint32_t instr);
    void executeFloatingPoint(uint32_t instr);
    void executeVector(uint32_t instr);
    void executeSyscall(uint32_t instr);
    
    // Common helpers
    uint32_t getBits(uint32_t value, int start, int end) const;
    void updateCR0(int64_t result);
    bool checkCondition(uint32_t bo, uint32_t bi);
};

} // namespace pxs3c

#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <memory>

namespace pxs3c {

class MemoryManager;

// SPU (Synergistic Processing Unit) - 128-bit SIMD processor
// PS3 has 6 SPUs (Cell processor)
// Local Store: 256KB per SPU
// Registers: 128 x 128-bit

union SPUVector {
    uint64_t u64[2];
    uint32_t u32[4];
    uint16_t u16[8];
    uint8_t u8[16];
    float f32[4];
    double f64[2];
    
    SPUVector() { u64[0] = 0; u64[1] = 0; }
};

struct SPURegisters {
    std::shared_ptr<std::array<SPUVector, 128>> regs;  // 128 x 128-bit registers (dynamic)
    
    uint32_t pc;      // Program Counter
    uint32_t sp;      // Stack Pointer (r1)
    uint32_t lr;      // Link Register
    uint32_t ctr;     // Count Register
    uint32_t status;  // SPU Status Register
    
    SPURegisters() : regs(std::make_shared<std::array<SPUVector, 128>>()) {
        pc = 0;
        sp = 0x3FFF0;  // Top of local store
        lr = 0;
        ctr = 0;
        status = 0;
    }
};

// Local Store (256KB per SPU)
constexpr uint32_t SPU_LOCAL_STORE_SIZE = 256 * 1024;
constexpr uint32_t SPU_LOCAL_STORE_BASE = 0x0;

// SPU instruction types
constexpr uint32_t SPU_OP_LOAD = 0x34;    // Load from memory
constexpr uint32_t SPU_OP_STORE = 0x24;   // Store to memory
constexpr uint32_t SPU_OP_IMMEDIATE = 0x20; // Immediate load
constexpr uint32_t SPU_OP_ARITHMETIC = 0x0C; // Arithmetic ops
constexpr uint32_t SPU_OP_BRANCH = 0x64;  // Branch

class SPUInterpreter {
public:
    SPUInterpreter(int id = 0);
    ~SPUInterpreter();

    bool init(std::shared_ptr<MemoryManager> mainMemory);
    void reset();
    
    // Local store
    std::vector<uint8_t>& getLocalStore() { return localStorage_; }
    
    // PC control
    void setPC(uint32_t pc) { regs_.pc = pc; }
    uint32_t getPC() const { return regs_.pc; }
    
    // Register access
    SPUVector getRegister(int n) const { if (n >= 0 && n < 128) return (*regs_.regs)[n]; return SPUVector(); }
    void setRegister(int n, const SPUVector& val) { if (n >= 0 && n < 128) (*regs_.regs)[n] = val; }
    
    // Execute
    void executeInstruction();
    void executeBlock(int maxInstructions = 1000);
    
    // Status
    void dumpRegisters() const;
    int getId() const { return id_; }
    bool isHalted() const { return halted_; }
    
private:
    int id_;
    SPURegisters regs_;
    std::vector<uint8_t> localStorage_;  // 256KB local store
    std::shared_ptr<MemoryManager> mainMemory_;
    bool halted_;
    
    // Instruction decoding
    void decodeAndExecute(uint32_t instruction);
    
    // Instruction groups
    void executeLoad(uint32_t instr);
    void executeStore(uint32_t instr);
    void executeArithmetic(uint32_t instr);
    void executeLogical(uint32_t instr);
    void executeBranch(uint32_t instr);
    void executeImmediate(uint32_t instr);
    
    // Helpers
    uint32_t getBits(uint32_t value, int start, int end) const;
    SPUVector add128(const SPUVector& a, const SPUVector& b);
    SPUVector sub128(const SPUVector& a, const SPUVector& b);
    SPUVector mul128(const SPUVector& a, const SPUVector& b);
    
    // Local store access
    SPUVector loadWord(uint32_t addr);
    void storeWord(uint32_t addr, const SPUVector& val);
};

} // namespace pxs3c

#include "cpu/SPUInterpreter.h"
#include "memory/MemoryManager.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>

namespace pxs3c {

SPUInterpreter::SPUInterpreter(int id)
    : id_(id), halted_(false) {
    try {
        // Initialize local store with lazy allocation attempt
        localStorage_.resize(SPU_LOCAL_STORE_SIZE, 0);
    } catch (const std::exception& e) {
        std::cerr << "SPU" << id_ << " failed to allocate local store: " << e.what() << std::endl;
        // Fallback: only allocate minimal local store (64KB instead of 256KB)
        localStorage_.resize(64 * 1024, 0);
    }
}

SPUInterpreter::~SPUInterpreter() {}

bool SPUInterpreter::init(std::shared_ptr<MemoryManager> mainMemory) {
    mainMemory_ = mainMemory;
    reset();
    std::cout << "SPU" << id_ << " initialized (256KB local store)" << std::endl;
    return true;
}

void SPUInterpreter::reset() {
    regs_ = SPURegisters();
    std::fill(localStorage_.begin(), localStorage_.end(), 0);
    halted_ = false;
}

uint32_t SPUInterpreter::getBits(uint32_t value, int start, int end) const {
    int count = end - start + 1;
    return (value >> (31 - end)) & ((1U << count) - 1);
}

SPUVector SPUInterpreter::add128(const SPUVector& a, const SPUVector& b) {
    SPUVector result;
    result.u64[0] = a.u64[0] + b.u64[0];
    result.u64[1] = a.u64[1] + b.u64[1];
    return result;
}

SPUVector SPUInterpreter::sub128(const SPUVector& a, const SPUVector& b) {
    SPUVector result;
    result.u64[0] = a.u64[0] - b.u64[0];
    result.u64[1] = a.u64[1] - b.u64[1];
    return result;
}

SPUVector SPUInterpreter::mul128(const SPUVector& a, const SPUVector& b) {
    SPUVector result;
    // Simple 32-bit multiplication
    for (int i = 0; i < 4; ++i) {
        result.u32[i] = a.u32[i] * b.u32[i];
    }
    return result;
}

SPUVector SPUInterpreter::loadWord(uint32_t addr) {
    if (addr + 16 > localStorage_.size()) {
        std::cerr << "SPU" << id_ << " load out of bounds: 0x" << std::hex << addr << std::dec << std::endl;
        return SPUVector();
    }
    
    SPUVector result;
    std::memcpy(&result, localStorage_.data() + addr, 16);
    return result;
}

void SPUInterpreter::storeWord(uint32_t addr, const SPUVector& val) {
    if (addr + 16 > localStorage_.size()) {
        std::cerr << "SPU" << id_ << " store out of bounds: 0x" << std::hex << addr << std::dec << std::endl;
        return;
    }
    
    std::memcpy(localStorage_.data() + addr, &val, 16);
}

void SPUInterpreter::executeInstruction() {
    if (halted_ || regs_.pc >= SPU_LOCAL_STORE_SIZE) {
        return;
    }
    
    // Load instruction from local store (big-endian)
    uint32_t instr = 0;
    if (regs_.pc + 4 <= SPU_LOCAL_STORE_SIZE) {
        std::memcpy(&instr, localStorage_.data() + regs_.pc, 4);
        // Convert from big-endian if needed
        instr = ((instr & 0xFF000000) >> 24) |
                ((instr & 0x00FF0000) >> 8) |
                ((instr & 0x0000FF00) << 8) |
                ((instr & 0x000000FF) << 24);
    }
    
    regs_.pc += 4;
    decodeAndExecute(instr);
}

void SPUInterpreter::executeBlock(int maxInstructions) {
    for (int i = 0; i < maxInstructions && !halted_; ++i) {
        executeInstruction();
    }
}

void SPUInterpreter::decodeAndExecute(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 7);
    
    switch (opcode) {
        case 0x18: // ai (add immediate)
        case 0x1C: // ahi (add half-word immediate)
        case 0x08: // a (add)
        case 0x0C: // ah (add half-word)
            executeArithmetic(instr);
            break;
            
        case 0x0F: // or, xor, and
        case 0x0E: // eqv
            executeLogical(instr);
            break;
            
        case 0x34: // lqd (load quad)
        case 0x24: // stqd (store quad)
            executeLoad(instr);
            break;
            
        case 0x64: // br, bra (branch)
        case 0x65: // brsl (branch with link)
            executeBranch(instr);
            break;
            
        case 0x20: // il (immediate load)
        case 0x21: // ilh (immediate load high)
        case 0x22: // ilhu
            executeImmediate(instr);
            break;
            
        default:
            std::cerr << "SPU" << id_ << " unknown instruction: 0x" << std::hex << instr 
                      << " at PC=0x" << (regs_.pc - 4) << std::dec << std::endl;
            halted_ = true;
            break;
    }
}

void SPUInterpreter::executeArithmetic(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 7);
    uint32_t rt = getBits(instr, 8, 12);   // Target register
    uint32_t ra = getBits(instr, 13, 17);  // Source A
    uint32_t rb = getBits(instr, 18, 22);  // Source B
    int32_t imm = (int16_t)getBits(instr, 18, 31);  // Immediate
    
    SPUVector result;
    
    switch (opcode) {
        case 0x18: // ai (add immediate)
            result = add128((*regs_.regs)[ra], SPUVector());
            result.u32[0] += imm;
            (*regs_.regs)[rt] = result;
            break;
            
        case 0x08: // a (add)
            result = add128((*regs_.regs)[ra], (*regs_.regs)[rb]);
            (*regs_.regs)[rt] = result;
            break;
            
        case 0x04: // s (subtract)
            result = sub128((*regs_.regs)[ra], (*regs_.regs)[rb]);
            (*regs_.regs)[rt] = result;
            break;
            
        case 0x14: // m (multiply)
            result = mul128((*regs_.regs)[ra], (*regs_.regs)[rb]);
            (*regs_.regs)[rt] = result;
            break;
            
        default:
            std::cerr << "SPU" << id_ << " unimplemented arithmetic opcode: " << opcode << std::endl;
            break;
    }
}

void SPUInterpreter::executeLogical(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 7);
    uint32_t rt = getBits(instr, 8, 12);
    uint32_t ra = getBits(instr, 13, 17);
    uint32_t rb = getBits(instr, 18, 22);
    
    SPUVector result = (*regs_.regs)[ra];
    
    switch (opcode) {
        case 0x0F: { // or, xor, and
            uint32_t subop = getBits(instr, 23, 31);
            if (subop == 0x0B8) { // or
                result.u64[0] |= (*regs_.regs)[rb].u64[0];
                result.u64[1] |= (*regs_.regs)[rb].u64[1];
            } else if (subop == 0x0B9) { // xor
                result.u64[0] ^= (*regs_.regs)[rb].u64[0];
                result.u64[1] ^= (*regs_.regs)[rb].u64[1];
            } else if (subop == 0x0BA) { // and
                result.u64[0] &= (*regs_.regs)[rb].u64[0];
                result.u64[1] &= (*regs_.regs)[rb].u64[1];
            }
            break;
        }
        default:
            break;
    }
    
    (*regs_.regs)[rt] = result;
}

void SPUInterpreter::executeLoad(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 7);
    uint32_t rt = getBits(instr, 8, 12);
    uint32_t ra = getBits(instr, 13, 17);
    int32_t offset = (int16_t)getBits(instr, 18, 31) << 2; // Offset is in quad-words
    
    if (opcode == 0x34) { // lqd
        uint32_t addr = (*regs_.regs)[ra].u32[0] + offset;
        SPUVector val = loadWord(addr);
        (*regs_.regs)[rt] = val;
    } else if (opcode == 0x24) { // stqd
        uint32_t addr = (*regs_.regs)[ra].u32[0] + offset;
        storeWord(addr, (*regs_.regs)[rt]);
    }
}

void SPUInterpreter::executeBranch(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 7);
    int32_t target = (int32_t)getBits(instr, 8, 31) << 2; // Offset is in quad-words
    
    if (opcode == 0x64) { // br
        regs_.pc += target - 4; // Adjust because PC was already incremented
    } else if (opcode == 0x65) { // brsl
        regs_.lr = regs_.pc;
        regs_.pc += target - 4;
    }
}

void SPUInterpreter::executeImmediate(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 7);
    uint32_t rt = getBits(instr, 8, 12);
    int32_t imm = (int16_t)getBits(instr, 13, 31);
    
    SPUVector result;
    
    switch (opcode) {
        case 0x20: // il (immediate load)
            result.u32[0] = imm;
            result.u32[1] = 0;
            result.u32[2] = 0;
            result.u32[3] = 0;
            break;
            
        case 0x21: // ilh (immediate load high)
            result.u32[0] = imm << 16;
            result.u32[1] = imm << 16;
            result.u32[2] = imm << 16;
            result.u32[3] = imm << 16;
            break;
            
        default:
            break;
    }
    
    (*regs_.regs)[rt] = result;
}

void SPUInterpreter::dumpRegisters() const {
    std::cout << "SPU" << id_ << " Registers:" << std::endl;
    std::cout << "PC=0x" << std::hex << std::setfill('0') << std::setw(8) << regs_.pc 
              << " LR=0x" << std::setw(8) << regs_.lr 
              << " CTR=0x" << std::setw(8) << regs_.ctr << std::dec << std::endl;
    
    for (int i = 0; i < 16; i += 4) {
        std::cout << "R" << std::setw(2) << i << "-" << std::setw(2) << (i+3) << ": ";
        for (int j = 0; j < 4; ++j) {
            std::cout << "0x" << std::hex << std::setfill('0') << std::setw(8) 
                      << (*regs_.regs)[i + j].u32[0] << " ";
        }
        std::cout << std::dec << std::endl;
    }
}

} // namespace pxs3c

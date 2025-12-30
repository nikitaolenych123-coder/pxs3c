#include "cpu/PPUInterpreter.h"
#include "memory/MemoryManager.h"
#include <iostream>
#include <iomanip>

namespace pxs3c {

PPUInterpreter::PPUInterpreter() : memory_(nullptr), halted_(false) {}

PPUInterpreter::~PPUInterpreter() {}

bool PPUInterpreter::init(MemoryManager* memory) {
    if (!memory) return false;
    memory_ = memory;
    reset();
    return true;
}

void PPUInterpreter::reset() {
    regs_ = PPURegisters();
    halted_ = false;
}

uint32_t PPUInterpreter::getBits(uint32_t value, int start, int end) const {
    int count = end - start + 1;
    return (value >> (31 - end)) & ((1U << count) - 1);
}

void PPUInterpreter::updateCR0(int64_t result) {
    uint32_t cr0 = 0;
    if (result < 0) cr0 |= 0x8;      // LT
    else if (result > 0) cr0 |= 0x4; // GT
    else cr0 |= 0x2;                  // EQ
    
    // SO bit from XER
    if (regs_.xer & 0x80000000) cr0 |= 0x1;
    
    regs_.cr = (regs_.cr & 0x0FFFFFFF) | (cr0 << 28);
}

bool PPUInterpreter::checkCondition(uint32_t bo, uint32_t bi) {
    // Simplified condition check
    bool ctr_ok = (bo & 0x04) || ((--regs_.ctr != 0) ^ ((bo & 0x02) != 0));
    bool cond_ok = (bo & 0x10) || (((regs_.cr >> (31 - bi)) & 1) == ((bo >> 3) & 1));
    return ctr_ok && cond_ok;
}

void PPUInterpreter::executeInstruction() {
    if (halted_ || !memory_) return;
    
    uint32_t instr = memory_->read32(regs_.pc);
    regs_.pc += 4;
    
    decodeAndExecute(instr);
}

void PPUInterpreter::executeBlock(int maxInstructions) {
    for (int i = 0; i < maxInstructions && !halted_; ++i) {
        executeInstruction();
    }
}

void PPUInterpreter::decodeAndExecute(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 5);
    
    // Decode primary opcode
    switch (opcode) {
        case 7:  // mulli
        case 8:  // subfic
        case 10: // cmpli
        case 11: // cmpi
        case 12: // addic
        case 13: // addic.
        case 14: // addi
        case 15: // addis
        case 31: // Extended arithmetic/logical
            executeArithmetic(instr);
            break;
            
        case 16: // bc (branch conditional)
        case 18: // b (branch)
        case 19: // bclr, bcctr
            executeBranch(instr);
            break;
            
        case 32: // lwz
        case 33: // lwzu
        case 34: // lbz
        case 35: // lbzu
        case 36: // stw
        case 37: // stwu
        case 38: // stb
        case 39: // stbu
        case 40: // lhz
        case 41: // lhzu
        case 42: // lha
        case 43: // lhau
        case 44: // sth
        case 45: // sthu
        case 58: // ld, ldu, lwa
        case 62: // std, stdu
            executeLoadStore(instr);
            break;
            
        case 17: // sc (system call)
            executeSystem(instr);
            break;
            
        case 59: // Floating point single
        case 63: // Floating point double
            executeFloatingPoint(instr);
            break;
            
        default:
            std::cerr << "Unknown instruction: 0x" << std::hex << instr 
                      << " at PC=0x" << (regs_.pc - 4) << std::dec << std::endl;
            halted_ = true;
            break;
    }
}

void PPUInterpreter::executeArithmetic(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 5);
    uint32_t rD = getBits(instr, 6, 10);
    uint32_t rA = getBits(instr, 11, 15);
    uint32_t rB = getBits(instr, 16, 20);
    int32_t simm = (int16_t)getBits(instr, 16, 31);
    uint32_t uimm = getBits(instr, 16, 31);
    
    switch (opcode) {
        case 14: // addi
            regs_.gpr[rD] = (rA == 0 ? 0 : regs_.gpr[rA]) + simm;
            break;
            
        case 15: // addis
            regs_.gpr[rD] = (rA == 0 ? 0 : regs_.gpr[rA]) + (simm << 16);
            break;
            
        case 24: // ori
            regs_.gpr[rA] = regs_.gpr[rD] | uimm;
            break;
            
        case 28: // andi.
            regs_.gpr[rA] = regs_.gpr[rD] & uimm;
            updateCR0(regs_.gpr[rA]);
            break;
            
        case 31: { // Extended
            uint32_t xop = getBits(instr, 21, 30);
            switch (xop) {
                case 266: // add
                    regs_.gpr[rD] = regs_.gpr[rA] + regs_.gpr[rB];
                    if (getBits(instr, 31, 31)) updateCR0(regs_.gpr[rD]);
                    break;
                    
                case 40: // subf
                    regs_.gpr[rD] = regs_.gpr[rB] - regs_.gpr[rA];
                    if (getBits(instr, 31, 31)) updateCR0(regs_.gpr[rD]);
                    break;
                    
                case 444: // or
                    regs_.gpr[rA] = regs_.gpr[rD] | regs_.gpr[rB];
                    break;
                    
                case 28: // and
                    regs_.gpr[rA] = regs_.gpr[rD] & regs_.gpr[rB];
                    break;
                    
                case 316: // xor
                    regs_.gpr[rA] = regs_.gpr[rD] ^ regs_.gpr[rB];
                    break;
                    
                default:
                    std::cerr << "Unknown extended arithmetic: xop=" << xop << std::endl;
                    break;
            }
            break;
        }
            
        default:
            std::cerr << "Unimplemented arithmetic opcode: " << opcode << std::endl;
            break;
    }
}

void PPUInterpreter::executeLogical(uint32_t instr) {
    // Handled in executeArithmetic for now
}

void PPUInterpreter::executeLoadStore(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 5);
    uint32_t rD = getBits(instr, 6, 10);
    uint32_t rA = getBits(instr, 11, 15);
    int32_t d = (int16_t)getBits(instr, 16, 31);
    
    uint64_t ea = (rA == 0 ? 0 : regs_.gpr[rA]) + d;
    
    switch (opcode) {
        case 32: // lwz
            regs_.gpr[rD] = memory_->read32(ea);
            break;
            
        case 34: // lbz
            regs_.gpr[rD] = memory_->read8(ea);
            break;
            
        case 40: // lhz
            regs_.gpr[rD] = memory_->read16(ea);
            break;
            
        case 36: // stw
            memory_->write32(ea, regs_.gpr[rD]);
            break;
            
        case 38: // stb
            memory_->write8(ea, regs_.gpr[rD] & 0xFF);
            break;
            
        case 44: // sth
            memory_->write16(ea, regs_.gpr[rD] & 0xFFFF);
            break;
            
        case 58: { // ld
            uint32_t ds = getBits(instr, 16, 29);
            uint32_t xop = getBits(instr, 30, 31);
            ea = (rA == 0 ? 0 : regs_.gpr[rA]) + (ds << 2);
            if (xop == 0) { // ld
                regs_.gpr[rD] = memory_->read64(ea);
            }
            break;
        }
            
        case 62: { // std
            uint32_t ds = getBits(instr, 16, 29);
            uint32_t xop = getBits(instr, 30, 31);
            ea = (rA == 0 ? 0 : regs_.gpr[rA]) + (ds << 2);
            if (xop == 0) { // std
                memory_->write64(ea, regs_.gpr[rD]);
            }
            break;
        }
            
        default:
            std::cerr << "Unimplemented load/store opcode: " << opcode << std::endl;
            break;
    }
}

void PPUInterpreter::executeBranch(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 5);
    
    switch (opcode) {
        case 18: { // b
            int32_t li = getBits(instr, 6, 29) << 2;
            if (li & 0x02000000) li |= 0xFC000000; // Sign extend
            bool aa = getBits(instr, 30, 30);
            bool lk = getBits(instr, 31, 31);
            
            if (lk) regs_.lr = regs_.pc;
            regs_.pc = aa ? li : (regs_.pc - 4 + li);
            break;
        }
            
        case 16: { // bc
            uint32_t bo = getBits(instr, 6, 10);
            uint32_t bi = getBits(instr, 11, 15);
            int32_t bd = getBits(instr, 16, 29) << 2;
            if (bd & 0x00008000) bd |= 0xFFFF0000; // Sign extend
            bool aa = getBits(instr, 30, 30);
            bool lk = getBits(instr, 31, 31);
            
            if (checkCondition(bo, bi)) {
                if (lk) regs_.lr = regs_.pc;
                regs_.pc = aa ? bd : (regs_.pc - 4 + bd);
            }
            break;
        }
            
        case 19: { // bclr, bcctr
            uint32_t xop = getBits(instr, 21, 30);
            uint32_t bo = getBits(instr, 6, 10);
            uint32_t bi = getBits(instr, 11, 15);
            bool lk = getBits(instr, 31, 31);
            
            if (xop == 16) { // bclr
                if (checkCondition(bo, bi)) {
                    uint64_t target = regs_.lr;
                    if (lk) regs_.lr = regs_.pc;
                    regs_.pc = target;
                }
            } else if (xop == 528) { // bcctr
                if (checkCondition(bo, bi)) {
                    uint64_t target = regs_.ctr;
                    if (lk) regs_.lr = regs_.pc;
                    regs_.pc = target;
                }
            }
            break;
        }
            
        default:
            std::cerr << "Unimplemented branch opcode: " << opcode << std::endl;
            break;
    }
}

void PPUInterpreter::executeSystem(uint32_t instr) {
    uint32_t opcode = getBits(instr, 0, 5);
    
    if (opcode == 17) { // sc
        uint32_t lev = getBits(instr, 20, 26);
        std::cout << "System call: lev=" << lev << " at PC=0x" << std::hex 
                  << (regs_.pc - 4) << std::dec << std::endl;
        
        // TODO: Handle LV1/LV2 syscalls
        // For now, just log and continue
    }
}

void PPUInterpreter::executeFloatingPoint(uint32_t instr) {
    // Stub: floating point instructions
    std::cerr << "Floating point instruction not implemented: 0x" << std::hex 
              << instr << std::dec << std::endl;
}

void PPUInterpreter::dumpRegisters() const {
    std::cout << "PPU Registers:" << std::endl;
    std::cout << "PC=0x" << std::hex << std::setfill('0') << std::setw(16) << regs_.pc 
              << " LR=0x" << std::setw(16) << regs_.lr 
              << " CTR=0x" << std::setw(16) << regs_.ctr << std::dec << std::endl;
    
    for (int i = 0; i < 32; i += 4) {
        std::cout << "GPR" << std::setw(2) << i << "-" << std::setw(2) << (i+3) << ": ";
        for (int j = 0; j < 4; ++j) {
            std::cout << "0x" << std::hex << std::setfill('0') << std::setw(16) 
                      << regs_.gpr[i + j] << " ";
        }
        std::cout << std::dec << std::endl;
    }
    
    std::cout << "CR=0x" << std::hex << regs_.cr 
              << " XER=0x" << regs_.xer << std::dec << std::endl;
}

} // namespace pxs3c

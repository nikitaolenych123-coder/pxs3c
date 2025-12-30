#include "cpu/PPUInterpreter.h"
#include "core/SyscallHandler.h"
#include "memory/MemoryManager.h"
#include <iostream>
#include <iomanip>

namespace pxs3c {

PPUInterpreter::PPUInterpreter() : memory_(nullptr), syscalls_(nullptr), halted_(false) {}

PPUInterpreter::~PPUInterpreter() {}

bool PPUInterpreter::init(MemoryManager* memory, SyscallHandler* syscalls) {
    if (!memory) return false;
    memory_ = memory;
    syscalls_ = syscalls;
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
        case 4:  // Vector/Altivec
            executeVector(instr);
            break;
            
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
            
        case 33: // lwzu (load with update)
            regs_.gpr[rD] = memory_->read32(ea);
            regs_.gpr[rA] = ea;
            break;
            
        case 34: // lbz
            regs_.gpr[rD] = memory_->read8(ea);
            break;
            
        case 35: // lbzu (load byte with update)
            regs_.gpr[rD] = memory_->read8(ea);
            regs_.gpr[rA] = ea;
            break;
            
        case 40: // lhz
            regs_.gpr[rD] = memory_->read16(ea);
            break;
            
        case 41: // lhzu (load half-word with update)
            regs_.gpr[rD] = memory_->read16(ea);
            regs_.gpr[rA] = ea;
            break;
            
        case 42: // lha (load half-word arithmetic - sign extended)
            {
                int16_t val = (int16_t)memory_->read16(ea);
                regs_.gpr[rD] = (int64_t)val;
            }
            break;
            
        case 43: // lhau (load half-word arithmetic with update)
            {
                int16_t val = (int16_t)memory_->read16(ea);
                regs_.gpr[rD] = (int64_t)val;
                regs_.gpr[rA] = ea;
            }
            break;
            
        case 36: // stw
            memory_->write32(ea, regs_.gpr[rD]);
            break;
            
        case 37: // stwu (store with update)
            memory_->write32(ea, regs_.gpr[rD]);
            regs_.gpr[rA] = ea;
            break;
            
        case 38: // stb
            memory_->write8(ea, regs_.gpr[rD] & 0xFF);
            break;
            
        case 39: // stbu (store byte with update)
            memory_->write8(ea, regs_.gpr[rD] & 0xFF);
            regs_.gpr[rA] = ea;
            break;
            
        case 44: // sth
            memory_->write16(ea, regs_.gpr[rD] & 0xFFFF);
            break;
            
        case 45: // sthu (store half-word with update)
            memory_->write16(ea, regs_.gpr[rD] & 0xFFFF);
            regs_.gpr[rA] = ea;
            break;
            
        case 58: { // ld
            uint32_t ds = getBits(instr, 16, 29);
            uint32_t xop = getBits(instr, 30, 31);
            ea = (rA == 0 ? 0 : regs_.gpr[rA]) + (ds << 2);
            if (xop == 0) { // ld
                regs_.gpr[rD] = memory_->read64(ea);
            } else if (xop == 1) { // ldu
                regs_.gpr[rD] = memory_->read64(ea);
                regs_.gpr[rA] = ea;
            }
            break;
        }
            
        case 62: { // std
            uint32_t ds = getBits(instr, 16, 29);
            uint32_t xop = getBits(instr, 30, 31);
            ea = (rA == 0 ? 0 : regs_.gpr[rA]) + (ds << 2);
            if (xop == 0) { // std
                memory_->write64(ea, regs_.gpr[rD]);
            } else if (xop == 1) { // stdu
                memory_->write64(ea, regs_.gpr[rD]);
                regs_.gpr[rA] = ea;
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
    
    if (opcode == 17) { // sc (system call)
        executeSyscall(instr);
    }
}

void PPUInterpreter::executeSyscall(uint32_t instr) {
    if (!syscalls_) {
        std::cout << "Syscall attempted but handler not initialized" << std::endl;
        return;
    }
    
    uint32_t lev = getBits(instr, 20, 26);
    uint64_t callNumber = regs_.gpr[0];
    
    SyscallContext ctx;
    ctx.r3 = regs_.gpr[3];
    ctx.r4 = regs_.gpr[4];
    ctx.r5 = regs_.gpr[5];
    ctx.r6 = regs_.gpr[6];
    ctx.r7 = regs_.gpr[7];
    ctx.r8 = regs_.gpr[8];
    ctx.r9 = regs_.gpr[9];
    ctx.r10 = regs_.gpr[10];
    ctx.r11 = regs_.gpr[11];
    ctx.returnValue = 0;
    ctx.handled = false;
    
    std::cout << "Syscall: call#=" << std::dec << callNumber 
              << " lev=" << lev << " r3=0x" << std::hex << ctx.r3 << std::dec << std::endl;
    
    // Call syscall handler
    bool success = syscalls_->handleSyscall(callNumber, ctx);
    
    // Set return value in r3
    regs_.gpr[3] = ctx.returnValue;
    
    if (!success) {
        std::cout << "Syscall failed or unhandled" << std::endl;
    }
}

void PPUInterpreter::executeFloatingPoint(uint32_t instr) {
    // Stub: floating point instructions
    uint32_t opcode = getBits(instr, 0, 5);
    uint32_t frt = getBits(instr, 6, 10);
    uint32_t fra = getBits(instr, 11, 15);
    uint32_t frb = getBits(instr, 16, 20);
    uint32_t xop = getBits(instr, 21, 30);
    
    if (opcode == 63) { // Floating point double
        switch (xop) {
            case 18: { // fdiv
                if (regs_.fpr[frb] != 0.0) {
                    regs_.fpr[frt] = regs_.fpr[fra] / regs_.fpr[frb];
                }
                break;
            }
            case 20: { // fsub
                regs_.fpr[frt] = regs_.fpr[fra] - regs_.fpr[frb];
                break;
            }
            case 21: { // fadd
                regs_.fpr[frt] = regs_.fpr[fra] + regs_.fpr[frb];
                break;
            }
            case 25: { // fmul
                regs_.fpr[frt] = regs_.fpr[fra] * regs_.fpr[frb];
                break;
            }
            case 72: { // fmr (move)
                regs_.fpr[frt] = regs_.fpr[frb];
                break;
            }
            default:
                std::cerr << "Unimplemented FP double opcode: xop=" << xop << std::endl;
                break;
        }
    }
}

void PPUInterpreter::executeVector(uint32_t instr) {
    // Vector/Altivec instructions (4x 32-bit floats)
    uint32_t opcode = getBits(instr, 0, 5);
    uint32_t vrt = getBits(instr, 6, 10);
    uint32_t vra = getBits(instr, 11, 15);
    uint32_t vrb = getBits(instr, 16, 20);
    uint32_t xop = getBits(instr, 21, 30);
    
    if (opcode == 4) { // Altivec ops
        switch (xop) {
            case 10: { // vaddfp (vector add float)
                for (int i = 0; i < 4; ++i) {
                    // Treat as float
                    float a = *(float*)&regs_.vr[vra].u32[i];
                    float b = *(float*)&regs_.vr[vrb].u32[i];
                    float result = a + b;
                    regs_.vr[vrt].u32[i] = *(uint32_t*)&result;
                }
                break;
            }
            case 74: { // vsubfp (vector subtract float)
                for (int i = 0; i < 4; ++i) {
                    float a = *(float*)&regs_.vr[vra].u32[i];
                    float b = *(float*)&regs_.vr[vrb].u32[i];
                    float result = a - b;
                    regs_.vr[vrt].u32[i] = *(uint32_t*)&result;
                }
                break;
            }
            case 34: { // vmulfp (vector multiply float) - placeholder
                for (int i = 0; i < 4; ++i) {
                    float a = *(float*)&regs_.vr[vra].u32[i];
                    float b = *(float*)&regs_.vr[vrb].u32[i];
                    float result = a * b;
                    regs_.vr[vrt].u32[i] = *(uint32_t*)&result;
                }
                break;
            }
            case 1028: { // vand
                for (int i = 0; i < 4; ++i) {
                    regs_.vr[vrt].u32[i] = regs_.vr[vra].u32[i] & regs_.vr[vrb].u32[i];
                }
                break;
            }
            case 1156: { // vor
                for (int i = 0; i < 4; ++i) {
                    regs_.vr[vrt].u32[i] = regs_.vr[vra].u32[i] | regs_.vr[vrb].u32[i];
                }
                break;
            }
            case 1220: { // vxor
                for (int i = 0; i < 4; ++i) {
                    regs_.vr[vrt].u32[i] = regs_.vr[vra].u32[i] ^ regs_.vr[vrb].u32[i];
                }
                break;
            }
            default:
                std::cerr << "Unimplemented vector opcode: xop=" << xop << std::endl;
                break;
        }
    }
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

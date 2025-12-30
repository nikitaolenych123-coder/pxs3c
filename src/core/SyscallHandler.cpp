#include "core/SyscallHandler.h"
#include "cpu/PPUInterpreter.h"
#include "memory/MemoryManager.h"
#include <iostream>

namespace pxs3c {

SyscallHandler::SyscallHandler()
    : ppu_(nullptr), memory_(nullptr), nextMemoryHandle_(1) {
    
    // Initialize syscall names
    syscallNames_[1] = "exit";
    syscallNames_[6] = "process_getpid";
    syscallNames_[82] = "process_prx_load_module";
    syscallNames_[83] = "process_prx_start_module";
    syscallNames_[202] = "sys_memory_allocate";
    syscallNames_[203] = "sys_memory_free";
    syscallNames_[205] = "sys_memory_get_user_memory_size";
    syscallNames_[348] = "sys_process_exit";
}

SyscallHandler::~SyscallHandler() {}

bool SyscallHandler::init(PPUInterpreter* ppu, MemoryManager* memory) {
    ppu_ = ppu;
    memory_ = memory;
    std::cout << "SyscallHandler initialized" << std::endl;
    return true;
}

void SyscallHandler::shutdown() {
    ppu_ = nullptr;
    memory_ = nullptr;
}

std::string SyscallHandler::getSyscallName(uint64_t callNumber) {
    auto it = syscallNames_.find(callNumber);
    if (it != syscallNames_.end()) {
        return it->second;
    }
    return "unknown_" + std::to_string(callNumber);
}

void SyscallHandler::logSyscall(uint64_t callNumber, const std::string& name) {
    std::cout << "Syscall: " << std::dec << callNumber << " (" << name << ")" << std::endl;
}

bool SyscallHandler::handleSyscall(uint64_t callNumber, SyscallContext& ctx) {
    logSyscall(callNumber, getSyscallName(callNumber));
    
    // LV2 syscalls (0-511)
    if (callNumber < 512) {
        switch (callNumber) {
            case 1:     return lv2_exit(ctx);
            case 6:     return lv2_process_getpid(ctx);
            case 82:    return lv2_process_prx_load_module(ctx);
            case 83:    return lv2_process_prx_start_module(ctx);
            case 202:   return lv2_sys_memory_allocate(ctx);
            case 203:   return lv2_sys_memory_free(ctx);
            case 205:   return lv2_sys_memory_get_user_memory_size(ctx);
            case 348:   return lv2_sys_process_exit(ctx);
            default:
                std::cout << "Unhandled LV2 syscall: " << callNumber << std::endl;
                return false;
        }
    }
    
    // LV1 syscalls (512+) - subtract 512 to get actual call number
    uint64_t lv1_call = callNumber - 512;
    switch (lv1_call) {
        case 1:     return lv1_get_version(ctx);
        default:
            std::cout << "Unhandled LV1 syscall: " << lv1_call << std::endl;
            return false;
    }
}

bool SyscallHandler::lv2_exit(SyscallContext& ctx) {
    std::cout << "LV2 exit with code: " << ctx.r3 << std::endl;
    ctx.returnValue = 0;
    return true;
}

bool SyscallHandler::lv2_process_getpid(SyscallContext& ctx) {
    // Return dummy PID
    ctx.returnValue = 1;
    return true;
}

bool SyscallHandler::lv2_process_prx_load_module(SyscallContext& ctx) {
    // r3 = path string address
    // r4 = flags
    // r5 = options address
    std::cout << "PRX load module requested" << std::endl;
    ctx.returnValue = 0x1;  // Module ID
    return true;
}

bool SyscallHandler::lv2_process_prx_start_module(SyscallContext& ctx) {
    // r3 = module ID
    // r4 = args address
    // r5 = arg size
    // r6 = entry address
    // r7 = result address
    std::cout << "PRX start module: id=" << ctx.r3 << std::endl;
    ctx.returnValue = 0;
    return true;
}

bool SyscallHandler::lv2_sys_memory_allocate(SyscallContext& ctx) {
    // r3 = size
    // r4 = flags
    // r5 = addr ptr
    uint64_t size = ctx.r3;
    std::cout << "Memory allocate: size=0x" << std::hex << size << std::dec << std::endl;
    
    // Dummy allocation at a fixed address
    uint64_t allocAddr = 0x20000000 + (nextMemoryHandle_ << 20);
    nextMemoryHandle_++;
    
    // Write address to memory if r5 provided
    if (ctx.r5 != 0 && memory_) {
        memory_->write64(ctx.r5, allocAddr);
    }
    
    ctx.returnValue = 0;  // Success
    return true;
}

bool SyscallHandler::lv2_sys_memory_free(SyscallContext& ctx) {
    // r3 = address
    std::cout << "Memory free: addr=0x" << std::hex << ctx.r3 << std::dec << std::endl;
    ctx.returnValue = 0;
    return true;
}

bool SyscallHandler::lv2_sys_memory_get_user_memory_size(SyscallContext& ctx) {
    // Return 256MB (0x10000000)
    ctx.returnValue = 0x10000000;
    return true;
}

bool SyscallHandler::lv2_sys_process_exit(SyscallContext& ctx) {
    std::cout << "Process exit with code: " << ctx.r3 << std::endl;
    ctx.returnValue = 0;
    return true;
}

bool SyscallHandler::lv1_get_version(SyscallContext& ctx) {
    // Return PS3 firmware version (dummy)
    ctx.returnValue = 0x0004B001;  // 4.81 firmware
    return true;
}

bool SyscallHandler::lv1_undocumented_function(SyscallContext& ctx) {
    std::cout << "Undocumented LV1 function called" << std::endl;
    ctx.returnValue = 0;
    return true;
}

} // namespace pxs3c

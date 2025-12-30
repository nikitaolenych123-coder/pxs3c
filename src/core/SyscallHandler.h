#pragma once

#include <cstdint>
#include <string>
#include <map>

namespace pxs3c {

class PPUInterpreter;
class MemoryManager;

// PS3 Syscalls (hypervisor calls)
// LV2 (OS level) syscalls: typically in range 0-80
// LV1 (hypervisor) syscalls: typically in range 80+

struct SyscallContext {
    uint64_t r3, r4, r5, r6, r7, r8, r9, r10, r11;  // Arguments
    uint64_t returnValue;  // r3
    bool handled;
};

class SyscallHandler {
public:
    SyscallHandler();
    ~SyscallHandler();

    bool init(PPUInterpreter* ppu, MemoryManager* memory);
    void shutdown();
    
    // Handle syscall
    bool handleSyscall(uint64_t callNumber, SyscallContext& ctx);
    
    // LV2 syscalls (kernel)
    bool lv2_exit(SyscallContext& ctx);
    bool lv2_process_getpid(SyscallContext& ctx);
    bool lv2_process_prx_load_module(SyscallContext& ctx);
    bool lv2_process_prx_start_module(SyscallContext& ctx);
    bool lv2_sys_process_exit(SyscallContext& ctx);
    bool lv2_sys_memory_allocate(SyscallContext& ctx);
    bool lv2_sys_memory_free(SyscallContext& ctx);
    bool lv2_sys_memory_get_user_memory_size(SyscallContext& ctx);
    
    // LV1 syscalls (hypervisor)
    bool lv1_get_version(SyscallContext& ctx);
    bool lv1_undocumented_function(SyscallContext& ctx);
    
    // Debug
    void logSyscall(uint64_t callNumber, const std::string& name);
    
private:
    PPUInterpreter* ppu_;
    MemoryManager* memory_;
    uint32_t nextMemoryHandle_;
    
    std::string getSyscallName(uint64_t callNumber);
    std::map<uint64_t, std::string> syscallNames_;
};

} // namespace pxs3c

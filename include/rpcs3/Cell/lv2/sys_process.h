#pragma once

#include <cstdint>

namespace rpcs3::lv2 {

// RPCS3 LV2 System Call Numbers
enum : uint64_t
{
    // Process management
    SYS_PROCESS_GETPID = 1,
    SYS_PROCESS_EXIT = 2,
    SYS_PROCESS_GET_NUMBER_OF_OBJECT = 9,
    SYS_PROCESS_GET_ID = 10,
    SYS_PROCESS_GET_PARAM = 11,
    
    // Thread management
    SYS_PPU_THREAD_CREATE = 50,
    SYS_PPU_THREAD_EXIT = 51,
    SYS_PPU_THREAD_JOIN = 52,
    SYS_PPU_THREAD_GET_ID = 53,
    SYS_PPU_THREAD_YIELD = 54,
    
    // Memory management
    SYS_MEMORY_ALLOCATE = 348,
    SYS_MEMORY_FREE = 349,
    SYS_MEMORY_GET_USER_MEMORY_SIZE = 350,
    
    // Time management
    SYS_TIME_GET_CURRENT_TIME = 141,
    SYS_TIME_GET_TIMEBASE_FREQUENCY = 147,
    
    // Mutex
    SYS_MUTEX_CREATE = 100,
    SYS_MUTEX_DESTROY = 101,
    SYS_MUTEX_LOCK = 102,
    SYS_MUTEX_UNLOCK = 103,
    
    // Condition variable
    SYS_COND_CREATE = 110,
    SYS_COND_DESTROY = 111,
    SYS_COND_WAIT = 112,
    SYS_COND_SIGNAL = 113,
    
    // Semaphore
    SYS_SEMAPHORE_CREATE = 120,
    SYS_SEMAPHORE_DESTROY = 121,
    SYS_SEMAPHORE_WAIT = 122,
    SYS_SEMAPHORE_POST = 123,
    
    // File system
    SYS_FS_OPEN = 801,
    SYS_FS_READ = 802,
    SYS_FS_WRITE = 803,
    SYS_FS_CLOSE = 804,
    SYS_FS_OPENDIR = 805,
    SYS_FS_READDIR = 806,
    SYS_FS_STAT = 807,
    SYS_FS_FSTAT = 808,
};

// Process parameters
struct sys_process_param_t
{
    uint32_t size;
    uint32_t magic;
    uint32_t version;
    uint32_t sdk_version;
    int32_t primary_prio;
    uint32_t primary_stacksize;
    uint32_t malloc_pagesize;
    uint32_t ppc_seg;
};

// RPCS3 Syscall Handler Interface
class lv2_syscall_handler
{
public:
    virtual ~lv2_syscall_handler() = default;
    
    // Process syscalls
    virtual uint64_t sys_process_getpid() = 0;
    virtual void sys_process_exit(int32_t status) = 0;
    
    // Thread syscalls
    virtual int32_t sys_ppu_thread_create(uint32_t* thread_id, uint64_t entry, uint64_t arg, 
                                          int32_t prio, uint32_t stacksize, uint64_t flags, const char* name) = 0;
    virtual void sys_ppu_thread_exit(uint64_t val) = 0;
    virtual uint64_t sys_ppu_thread_get_id() = 0;
    
    // Memory syscalls
    virtual int32_t sys_memory_allocate(uint32_t size, uint64_t flags, uint32_t* addr) = 0;
    virtual int32_t sys_memory_free(uint32_t addr) = 0;
    
    // Time syscalls
    virtual uint64_t sys_time_get_current_time() = 0;
    virtual uint64_t sys_time_get_timebase_frequency() = 0;
    
    // Dispatch syscall
    virtual uint64_t dispatch(uint64_t syscall_number, uint64_t arg1, uint64_t arg2, 
                             uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) = 0;
};

} // namespace rpcs3::lv2

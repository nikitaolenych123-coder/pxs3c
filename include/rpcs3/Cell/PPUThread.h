#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace rpcs3 {

// RPCS3 PPU Thread State
enum class ppu_thread_status : uint32_t
{
    idle,
    runnable,
    running,
    waiting,
    suspended,
    stopped,
    zombie
};

// PPU GPRs (General Purpose Registers)
struct ppu_gpr_t
{
    uint64_t value;
    
    operator uint64_t() const { return value; }
    ppu_gpr_t& operator=(uint64_t val) { value = val; return *this; }
};

// PPU FPRs (Floating Point Registers)
struct alignas(8) ppu_fpr_t
{
    double value;
    
    operator double() const { return value; }
    ppu_fpr_t& operator=(double val) { value = val; return *this; }
};

// PPU VRs (Vector Registers - Altivec)
struct alignas(16) ppu_vr_t
{
    uint32_t _u32[4];
    
    uint32_t u32(int index) const { return _u32[3 - index]; }
    void set_u32(int index, uint32_t value) { _u32[3 - index] = value; }
};

// PPU Thread Context (RPCS3 style)
struct ppu_thread_context
{
    // GPRs (r0-r31)
    ppu_gpr_t gpr[32]{};
    
    // FPRs (f0-f31)
    ppu_fpr_t fpr[32]{};
    
    // VRs (v0-v31)
    ppu_vr_t vr[32]{};
    
    // Special registers
    uint64_t pc{0};      // Program Counter
    uint64_t lr{0};      // Link Register
    uint64_t ctr{0};     // Count Register
    uint32_t cr{0};      // Condition Register
    uint32_t xer{0};     // Integer Exception Register
    uint32_t fpscr{0};   // FP Status Control Register
    uint32_t vscr{0};    // Vector Status Control Register
    
    // Thread info
    uint32_t id{0};
    uint64_t stack_addr{0};
    uint32_t stack_size{0};
    ppu_thread_status status{ppu_thread_status::idle};
    
    // Priority and scheduling
    int32_t priority{0};
    uint64_t cycles{0};
};

// PPU Thread (RPCS3 architecture)
class ppu_thread
{
public:
    ppu_thread() = default;
    ~ppu_thread() = default;
    
    // Initialize thread
    bool init(uint32_t thread_id, uint64_t entry, uint64_t stack);
    
    // Execute one instruction
    void step();
    
    // Execute block of instructions
    void run_block(int max_instructions);
    
    // Thread control
    void suspend();
    void resume();
    void stop();
    
    // Get context
    ppu_thread_context& get_context() { return ctx_; }
    const ppu_thread_context& get_context() const { return ctx_; }
    
    // Status
    bool is_running() const { return ctx_.status == ppu_thread_status::running; }
    bool is_stopped() const { return ctx_.status == ppu_thread_status::stopped; }
    
    // RPCS3 syscall interface
    uint64_t syscall(uint64_t code);
    
private:
    ppu_thread_context ctx_;
    
    // Instruction execution
    void execute_instruction(uint32_t instr);
    
    // Opcode handlers (RPCS3 style)
    void execute_group_4(uint32_t op);
    void execute_group_19(uint32_t op);
    void execute_group_31(uint32_t op);
    void execute_group_59(uint32_t op);
    void execute_group_63(uint32_t op);
};

} // namespace rpcs3

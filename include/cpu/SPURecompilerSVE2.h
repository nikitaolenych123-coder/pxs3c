#pragma once

#include <cstdint>
#include <vector>
#include <arm_sve.h>

namespace PXS3C {

/**
 * @brief High-performance SPU JIT Recompiler using ARMv9 SVE2
 * Targets 4x speed improvement over traditional ASIMD implementations
 */
class SPURecompilerSVE2 {
public:
    SPURecompilerSVE2();
    ~SPURecompilerSVE2();

    /**
     * @brief Initialize SVE2 recompiler with runtime detection
     * @return true if SVE2 is available on this CPU
     */
    bool initialize();

    /**
     * @brief Compile SPU instruction block using SVE2 vectorization
     * @param spu_pc Starting PC address in SPU local store
     * @param instructions Pointer to SPU instructions (big-endian)
     * @param count Number of instructions to compile
     * @return Pointer to compiled ARM64 code
     */
    void* compileBlock(uint32_t spu_pc, const uint32_t* instructions, size_t count);

    /**
     * @brief Execute compiled SPU code block
     * @param block_ptr Pointer returned from compileBlock()
     * @param spu_context SPU register context (128 x 128-bit registers)
     */
    void executeBlock(void* block_ptr, void* spu_context);

    /**
     * @brief Check if SVE2 acceleration is available
     */
    bool isSVE2Available() const { return sve2_available_; }

    /**
     * @brief Get optimal vector length for this CPU (128-2048 bits)
     */
    uint32_t getVectorLength() const { return vector_length_; }

private:
    bool sve2_available_;
    uint32_t vector_length_; // SVE vector length in bits
    
    // JIT code cache
    struct CodeBlock {
        void* code;
        size_t size;
        uint32_t spu_pc;
    };
    std::vector<CodeBlock> code_cache_;

    // SVE2 optimization helpers
    void emitSPUVectorOp(uint32_t opcode, svfloat32_t* regs);
    void emitSPUShuffleOp(uint32_t opcode, svuint8_t* regs);
    void emitSPUArithmeticOp(uint32_t opcode, svint32_t* regs);
};

} // namespace PXS3C

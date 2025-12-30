#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <memory>

namespace pxs3c {

// PS3 Memory Map (simplified)
// 0x00010000 - 0x10000000: Main RAM (256MB)
// 0x20000000 - 0x30000000: User space
// 0xC0000000 - 0xD0000000: RSX memory (256MB)
// 0xD0000000 - 0xE0000000: MMIO

constexpr uint64_t MAIN_MEMORY_BASE = 0x00010000;
constexpr uint64_t MAIN_MEMORY_SIZE = 0x10000000; // 256MB
constexpr uint64_t USER_MEMORY_BASE = 0x20000000;
constexpr uint64_t USER_MEMORY_SIZE = 0x10000000; // 256MB
constexpr uint64_t RSX_MEMORY_BASE = 0xC0000000;
constexpr uint64_t RSX_MEMORY_SIZE = 0x10000000; // 256MB

// Memory protection flags (matches ELF p_flags)
constexpr uint32_t MEM_PROT_EXEC = 0x1;
constexpr uint32_t MEM_PROT_WRITE = 0x2;
constexpr uint32_t MEM_PROT_READ = 0x4;

struct MemoryRegion {
    uint64_t base;
    uint64_t size;
    uint32_t flags;
    std::shared_ptr<std::vector<uint8_t>> data;  // Use shared_ptr for lazy allocation
};

class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();

    bool init();
    void shutdown();

    // Memory mapping
    bool mapRegion(uint64_t vaddr, uint64_t size, uint32_t flags);
    bool unmapRegion(uint64_t vaddr);
    MemoryRegion* getRegion(uint64_t vaddr);

    // Memory access
    bool read(uint64_t vaddr, void* dst, size_t size);
    bool write(uint64_t vaddr, const void* src, size_t size);

    // Typed reads (with endian swap for PS3 big-endian)
    uint8_t read8(uint64_t vaddr);
    uint16_t read16(uint64_t vaddr);
    uint32_t read32(uint64_t vaddr);
    uint64_t read64(uint64_t vaddr);

    // Typed writes (with endian swap for PS3 big-endian)
    void write8(uint64_t vaddr, uint8_t value);
    void write16(uint64_t vaddr, uint16_t value);
    void write32(uint64_t vaddr, uint32_t value);
    void write64(uint64_t vaddr, uint64_t value);

    // Direct pointer access (unsafe, for performance)
    uint8_t* getPointer(uint64_t vaddr);

    // Stats
    size_t getTotalMapped() const;
    void dumpRegions() const;

private:
    std::map<uint64_t, MemoryRegion> regions_;
    bool initialized_;
    
    // Lazy allocation helper
    bool allocateOnDemand(uint64_t vaddr);

    uint16_t swapEndian16(uint16_t val);
    uint32_t swapEndian32(uint32_t val);
    uint64_t swapEndian64(uint64_t val);
};

} // namespace pxs3c

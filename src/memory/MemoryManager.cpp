#include "memory/MemoryManager.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace pxs3c {

MemoryManager::MemoryManager() : initialized_(false) {}

MemoryManager::~MemoryManager() {
    shutdown();
}

bool MemoryManager::init() {
    if (initialized_) return true;

    // Pre-allocate main memory regions
    std::cout << "Initializing PS3 memory map..." << std::endl;

    try {
        // Main RAM (256MB) - use reserve+resize to avoid constructor overhead
        MemoryRegion mainRam;
        mainRam.base = MAIN_MEMORY_BASE;
        mainRam.size = MAIN_MEMORY_SIZE;
        mainRam.flags = MEM_PROT_READ | MEM_PROT_WRITE;
        
        // Android-safe allocation: reserve first, then resize in smaller chunks
        mainRam.data.reserve(MAIN_MEMORY_SIZE);
        const size_t chunkSize = 16 * 1024 * 1024; // 16MB chunks
        for (size_t offset = 0; offset < MAIN_MEMORY_SIZE; offset += chunkSize) {
            size_t currentChunk = std::min(chunkSize, MAIN_MEMORY_SIZE - offset);
            mainRam.data.resize(offset + currentChunk, 0);
        }
        
        regions_[mainRam.base] = std::move(mainRam);

        std::cout << "Main RAM: 0x" << std::hex << MAIN_MEMORY_BASE 
                  << " - 0x" << (MAIN_MEMORY_BASE + MAIN_MEMORY_SIZE) << std::dec << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to allocate memory: " << e.what() << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}

void MemoryManager::shutdown() {
    if (!initialized_) return;
    regions_.clear();
    initialized_ = false;
}

bool MemoryManager::mapRegion(uint64_t vaddr, uint64_t size, uint32_t flags) {
    // Check for overlaps
    for (const auto& [base, region] : regions_) {
        uint64_t end = base + region.size;
        uint64_t newEnd = vaddr + size;
        
        if ((vaddr >= base && vaddr < end) || (newEnd > base && newEnd <= end)) {
            std::cerr << "Memory region overlap at 0x" << std::hex << vaddr << std::dec << std::endl;
            return false;
        }
    }

    MemoryRegion region;
    region.base = vaddr;
    region.size = size;
    region.flags = flags;
    region.data.resize(size, 0);
    
    regions_[vaddr] = std::move(region);
    
    std::cout << "Mapped region: 0x" << std::hex << vaddr << " size=0x" << size 
              << " flags=0x" << flags << std::dec << std::endl;
    
    return true;
}

bool MemoryManager::unmapRegion(uint64_t vaddr) {
    auto it = regions_.find(vaddr);
    if (it == regions_.end()) {
        return false;
    }
    regions_.erase(it);
    return true;
}

MemoryRegion* MemoryManager::getRegion(uint64_t vaddr) {
    for (auto& [base, region] : regions_) {
        if (vaddr >= base && vaddr < base + region.size) {
            return &region;
        }
    }
    return nullptr;
}

bool MemoryManager::read(uint64_t vaddr, void* dst, size_t size) {
    MemoryRegion* region = getRegion(vaddr);
    if (!region) {
        std::cerr << "Read from unmapped memory: 0x" << std::hex << vaddr << std::dec << std::endl;
        return false;
    }

    if (!(region->flags & MEM_PROT_READ)) {
        std::cerr << "Read from non-readable memory: 0x" << std::hex << vaddr << std::dec << std::endl;
        return false;
    }

    uint64_t offset = vaddr - region->base;
    if (offset + size > region->size) {
        std::cerr << "Read out of bounds: 0x" << std::hex << vaddr << std::dec << std::endl;
        return false;
    }

    std::memcpy(dst, region->data.data() + offset, size);
    return true;
}

bool MemoryManager::write(uint64_t vaddr, const void* src, size_t size) {
    MemoryRegion* region = getRegion(vaddr);
    if (!region) {
        std::cerr << "Write to unmapped memory: 0x" << std::hex << vaddr << std::dec << std::endl;
        return false;
    }

    if (!(region->flags & MEM_PROT_WRITE)) {
        std::cerr << "Write to non-writable memory: 0x" << std::hex << vaddr << std::dec << std::endl;
        return false;
    }

    uint64_t offset = vaddr - region->base;
    if (offset + size > region->size) {
        std::cerr << "Write out of bounds: 0x" << std::hex << vaddr << std::dec << std::endl;
        return false;
    }

    std::memcpy(region->data.data() + offset, src, size);
    return true;
}

uint16_t MemoryManager::swapEndian16(uint16_t val) {
    return ((val & 0xFF00) >> 8) | ((val & 0x00FF) << 8);
}

uint32_t MemoryManager::swapEndian32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x000000FF) << 24);
}

uint64_t MemoryManager::swapEndian64(uint64_t val) {
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8)  |
           ((val & 0x00000000FF000000ULL) << 8)  |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
}

uint8_t MemoryManager::read8(uint64_t vaddr) {
    uint8_t val = 0;
    read(vaddr, &val, 1);
    return val;
}

uint16_t MemoryManager::read16(uint64_t vaddr) {
    uint16_t val = 0;
    read(vaddr, &val, 2);
    return swapEndian16(val); // PS3 is big-endian
}

uint32_t MemoryManager::read32(uint64_t vaddr) {
    uint32_t val = 0;
    read(vaddr, &val, 4);
    return swapEndian32(val); // PS3 is big-endian
}

uint64_t MemoryManager::read64(uint64_t vaddr) {
    uint64_t val = 0;
    read(vaddr, &val, 8);
    return swapEndian64(val); // PS3 is big-endian
}

void MemoryManager::write8(uint64_t vaddr, uint8_t value) {
    write(vaddr, &value, 1);
}

void MemoryManager::write16(uint64_t vaddr, uint16_t value) {
    uint16_t swapped = swapEndian16(value); // PS3 is big-endian
    write(vaddr, &swapped, 2);
}

void MemoryManager::write32(uint64_t vaddr, uint32_t value) {
    uint32_t swapped = swapEndian32(value); // PS3 is big-endian
    write(vaddr, &swapped, 4);
}

void MemoryManager::write64(uint64_t vaddr, uint64_t value) {
    uint64_t swapped = swapEndian64(value); // PS3 is big-endian
    write(vaddr, &swapped, 8);
}

uint8_t* MemoryManager::getPointer(uint64_t vaddr) {
    MemoryRegion* region = getRegion(vaddr);
    if (!region) return nullptr;
    
    uint64_t offset = vaddr - region->base;
    return region->data.data() + offset;
}

size_t MemoryManager::getTotalMapped() const {
    size_t total = 0;
    for (const auto& [base, region] : regions_) {
        total += region.size;
    }
    return total;
}

void MemoryManager::dumpRegions() const {
    std::cout << "Memory Regions (" << regions_.size() << "):" << std::endl;
    for (const auto& [base, region] : regions_) {
        std::cout << "  0x" << std::hex << base << " - 0x" << (base + region.size)
                  << " (" << std::dec << (region.size / 1024 / 1024) << " MB)"
                  << " flags=0x" << std::hex << region.flags << std::dec << std::endl;
    }
}

} // namespace pxs3c

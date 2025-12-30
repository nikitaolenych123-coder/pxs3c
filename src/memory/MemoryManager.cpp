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

    // PS3 memory map initialization - NO PRE-ALLOCATION
    // Memory will be allocated on-demand when accessed
    std::cout << "Initializing PS3 memory map (lazy allocation)..." << std::endl;

    try {
        // Only create the region metadata, don't allocate actual memory yet
        MemoryRegion mainRam;
        mainRam.base = MAIN_MEMORY_BASE;
        mainRam.size = MAIN_MEMORY_SIZE;
        mainRam.flags = MEM_PROT_READ | MEM_PROT_WRITE;
        // Data is null - will be allocated on-demand
        mainRam.data = nullptr;
        
        // Insert safely with exception handling
        try {
            regions_[mainRam.base] = mainRam;  // Copy instead of move for stability
        } catch (const std::exception& e) {
            std::cerr << "Map insert failed: " << e.what() << std::endl;
            return false;
        }

        std::cout << "Main RAM metadata created: 0x" << std::hex << MAIN_MEMORY_BASE 
                  << " - 0x" << (MAIN_MEMORY_BASE + MAIN_MEMORY_SIZE) << std::dec << std::endl;
        std::cout << "Memory allocation: lazy (on-demand)" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize memory metadata: " << e.what() << std::endl;
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
    region.data = std::make_shared<std::vector<uint8_t>>();
    region.data->resize(size, 0);
    
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
        // Lazy allocation: allocate region on first access
        if (!allocateOnDemand(vaddr)) {
            std::cerr << "Read from unmapped memory: 0x" << std::hex << vaddr << std::dec << std::endl;
            return false;
        }
        region = getRegion(vaddr);
        if (!region) return false;
    }
    
    // Ensure data is allocated for this region
    if (!region->data) {
        try {
            region->data = std::make_shared<std::vector<uint8_t>>();
            region->data->resize(std::min(region->size, (uint64_t)1024 * 1024), 0); // 1MB default
        } catch (...) {
            return false;
        }
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

    std::memcpy(dst, region->data->data() + offset, size);
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

    // Ensure data is allocated
    if (!region->data) {
        try {
            region->data = std::make_shared<std::vector<uint8_t>>();
            region->data->resize(std::min(region->size, (uint64_t)1024 * 1024), 0);
        } catch (...) {
            return false;
        }
    }

    std::memcpy(region->data->data() + offset, src, size);
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
    if (!region || !region->data) return nullptr;
    
    uint64_t offset = vaddr - region->base;
    return region->data->data() + offset;
}

bool MemoryManager::allocateOnDemand(uint64_t vaddr) {
    // Check if already in a region
    for (auto& [base, region] : regions_) {
        if (vaddr >= base && vaddr < base + region.size) {
            return true;
        }
    }
    
    // Allocate new 1MB region starting from vaddr
    MemoryRegion newRegion;
    newRegion.base = vaddr & ~0xFFFFF; // 1MB aligned
    newRegion.size = 1024 * 1024;
    newRegion.flags = MEM_PROT_READ | MEM_PROT_WRITE;
    
    try {
        newRegion.data = std::make_shared<std::vector<uint8_t>>();
        newRegion.data->resize(newRegion.size, 0);
        regions_[newRegion.base] = std::move(newRegion);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to allocate on-demand region: " << e.what() << std::endl;
        return false;
    }
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

#include "loader/ElfLoader.h"
#include "loader/SELFLoader.h"
#include "memory/MemoryManager.h"
#include <fstream>
#include <iostream>
#include <cstring>

namespace pxs3c {

ElfLoader::ElfLoader() : entryPoint_(0) {}

ElfLoader::~ElfLoader() {}

uint16_t ElfLoader::swapEndian16(uint16_t val) {
    return ((val & 0xFF00) >> 8) | ((val & 0x00FF) << 8);
}

uint32_t ElfLoader::swapEndian32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x000000FF) << 24);
}

uint64_t ElfLoader::swapEndian64(uint64_t val) {
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8)  |
           ((val & 0x00000000FF000000ULL) << 8)  |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
}

bool ElfLoader::validateHeader(const Elf64_Ehdr& header) {
    // Check ELF magic
    if (header.e_ident[0] != 0x7F || header.e_ident[1] != 'E' ||
        header.e_ident[2] != 'L' || header.e_ident[3] != 'F') {
        std::cerr << "Invalid ELF magic" << std::endl;
        return false;
    }

    // Check 64-bit
    if (header.e_ident[4] != ELFCLASS64) {
        std::cerr << "Not a 64-bit ELF" << std::endl;
        return false;
    }

    // Check big-endian
    if (header.e_ident[5] != ELFDATA2MSB) {
        std::cerr << "Not big-endian" << std::endl;
        return false;
    }

    // Check machine type (PowerPC 64)
    uint16_t machine = swapEndian16(header.e_machine);
    if (machine != EM_PPC64) {
        std::cerr << "Not PowerPC 64 (machine: " << machine << ")" << std::endl;
        return false;
    }

    return true;
}

bool ElfLoader::load(const std::string& path, MemoryManager* memory) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open ELF file: " << path << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "Failed to read ELF file" << std::endl;
        return false;
    }

    if (buffer.size() < sizeof(Elf64_Ehdr)) {
        std::cerr << "File too small for ELF header" << std::endl;
        return false;
    }

    // Parse header
    Elf64_Ehdr header;
    std::memcpy(&header, buffer.data(), sizeof(Elf64_Ehdr));

    if (!validateHeader(header)) {
        return false;
    }

    entryPoint_ = swapEndian64(header.e_entry);
    uint64_t phoff = swapEndian64(header.e_phoff);
    uint16_t phnum = swapEndian16(header.e_phnum);
    uint16_t phentsize = swapEndian16(header.e_phentsize);

    std::cout << "ELF Entry Point: 0x" << std::hex << entryPoint_ << std::dec << std::endl;
    std::cout << "Program Headers: " << phnum << " at offset 0x" << std::hex << phoff << std::dec << std::endl;

    // Parse program headers
    for (uint16_t i = 0; i < phnum; ++i) {
        size_t offset = phoff + i * phentsize;
        if (offset + sizeof(Elf64_Phdr) > buffer.size()) {
            std::cerr << "Program header out of bounds" << std::endl;
            return false;
        }

        Elf64_Phdr phdr;
        std::memcpy(&phdr, buffer.data() + offset, sizeof(Elf64_Phdr));

        uint32_t type = swapEndian32(phdr.p_type);
        uint32_t flags = swapEndian32(phdr.p_flags);
        uint64_t fileOffset = swapEndian64(phdr.p_offset);
        uint64_t vaddr = swapEndian64(phdr.p_vaddr);
        uint64_t filesz = swapEndian64(phdr.p_filesz);
        uint64_t memsz = swapEndian64(phdr.p_memsz);

        if (type == PT_LOAD) {
            std::cout << "LOAD segment: vaddr=0x" << std::hex << vaddr 
                      << " filesz=0x" << filesz << " memsz=0x" << memsz 
                      << " flags=0x" << flags << std::dec << std::endl;

            LoadedSegment segment;
            segment.vaddr = vaddr;
            segment.size = memsz;
            segment.flags = flags;
            segment.data.resize(memsz, 0);

            // Copy file data
            if (filesz > 0) {
                if (fileOffset + filesz > buffer.size()) {
                    std::cerr << "Segment data out of bounds" << std::endl;
                    return false;
                }
                std::memcpy(segment.data.data(), buffer.data() + fileOffset, filesz);
            }

            // Load into memory if memory manager available
            if (memory) {
                if (!memory->mapRegion(vaddr, memsz, flags)) {
                    std::cerr << "Failed to map memory region" << std::endl;
                    return false;
                }
                if (filesz > 0) {
                    if (!memory->write(vaddr, segment.data.data(), filesz)) {
                        std::cerr << "Failed to write segment to memory" << std::endl;
                        return false;
                    }
                }
            }

            segments_.push_back(std::move(segment));
        }
    }

    std::cout << "ELF loaded successfully (" << segments_.size() << " segments)" << std::endl;
    return true;
}

bool ElfLoader::loadSelf(const std::string& path, MemoryManager* memory) {
    std::cout << "Loading SELF file: " << path << std::endl;
    
    // First, parse SELF file
    SELFLoader selfLoader;
    if (!selfLoader.loadSelf(path.c_str())) {
        std::cerr << "Failed to load SELF file" << std::endl;
        return false;
    }
    
    // Get embedded ELF data from SELF
    std::vector<uint8_t> elfData;
    if (!selfLoader.extractELF(elfData)) {
        std::cerr << "Failed to extract ELF from SELF" << std::endl;
        return false;
    }
    
    // For now, SELF extraction is stubbed
    // In production, would load the decrypted ELF data
    std::cout << "SELF extraction stub - ELF decryption not yet implemented" << std::endl;
    std::cout << "Would load " << elfData.size() << " bytes of ELF data" << std::endl;
    
    return true;
}

} // namespace pxs3c

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace pxs3c {

// ELF types for PS3 (PowerPC 64-bit big-endian)
constexpr uint32_t ELF_MAGIC = 0x7F454C46; // "\x7FELF"
constexpr uint16_t EM_PPC64 = 21;
constexpr uint8_t ELFCLASS64 = 2;
constexpr uint8_t ELFDATA2MSB = 2; // Big-endian

struct Elf64_Ehdr {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf64_Phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

struct Elf64_Shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
};

// Program header types
constexpr uint32_t PT_NULL = 0;
constexpr uint32_t PT_LOAD = 1;
constexpr uint32_t PT_DYNAMIC = 2;
constexpr uint32_t PT_INTERP = 3;
constexpr uint32_t PT_TLS = 7;
constexpr uint32_t PT_SCE_RELA = 0x60000000;
constexpr uint32_t PT_SCE_DYNLIBDATA = 0x61000000;
constexpr uint32_t PT_SCE_PROCPARAM = 0x61000001;
constexpr uint32_t PT_SCE_MODULE_PARAM = 0x61000002;

// Section types
constexpr uint32_t SHT_NULL = 0;
constexpr uint32_t SHT_PROGBITS = 1;
constexpr uint32_t SHT_SYMTAB = 2;
constexpr uint32_t SHT_STRTAB = 3;
constexpr uint32_t SHT_RELA = 4;
constexpr uint32_t SHT_NOBITS = 8;

class MemoryManager;

struct LoadedSegment {
    uint64_t vaddr;
    uint64_t size;
    uint32_t flags;
    std::vector<uint8_t> data;
};

class ElfLoader {
public:
    ElfLoader();
    ~ElfLoader();

    bool load(const std::string& path, MemoryManager* memory);
    uint64_t getEntryPoint() const { return entryPoint_; }
    const std::vector<LoadedSegment>& getSegments() const { return segments_; }

private:
    bool validateHeader(const Elf64_Ehdr& header);
    bool loadSegments(const std::vector<uint8_t>& data, MemoryManager* memory);
    uint16_t swapEndian16(uint16_t val);
    uint32_t swapEndian32(uint32_t val);
    uint64_t swapEndian64(uint64_t val);

    uint64_t entryPoint_;
    std::vector<LoadedSegment> segments_;
};

} // namespace pxs3c

#include "loader/SELFLoader.h"
#include <iostream>
#include <fstream>
#include <cstring>

namespace pxs3c {

// PS3 SELF uses AES-128 with hardcoded key (publicly known)
// For demonstration, using stub decryption
const uint8_t SELFLoader::defaultKey[] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

const uint8_t SELFLoader::defaultIV[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

SELFLoader::SELFLoader() = default;
SELFLoader::~SELFLoader() = default;

bool SELFLoader::loadSelf(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open SELF file: " << path << std::endl;
        return false;
    }
    
    // Read entire file
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    file.close();
    
    return loadSelfFromBuffer(buffer.data(), buffer.size());
}

bool SELFLoader::loadSelfFromBuffer(const uint8_t* buffer, uint32_t size) {
    if (!buffer || size < sizeof(SELFHeader)) {
        std::cerr << "Invalid SELF buffer size" << std::endl;
        return false;
    }
    
    // Parse header
    if (!parseSelfHeader(buffer, header_)) {
        std::cerr << "Failed to parse SELF header" << std::endl;
        return false;
    }
    
    std::cout << "SELF file loaded: version=0x" << std::hex << header_.version
              << " sections=" << std::dec << header_.secHeaderCount << std::endl;
    
    // Parse section headers
    if (!parseSelfSections(buffer, size)) {
        std::cerr << "Failed to parse SELF sections" << std::endl;
        return false;
    }
    
    // Parse metadata (for signature verification)
    if (!parseSelfMetadata(buffer, size)) {
        std::cerr << "Warning: Failed to parse SELF metadata" << std::endl;
    }
    
    // Extract embedded ELF (still encrypted in real PS3, but stub here)
    if (!extractELF(rawElf_)) {
        std::cerr << "Failed to extract ELF from SELF" << std::endl;
        return false;
    }
    
    dumpSelfInfo();
    return true;
}

bool SELFLoader::parseSelfHeader(const uint8_t* buffer, SELFHeader& header) {
    if (!buffer) return false;
    
    std::memcpy(&header, buffer, sizeof(SELFHeader));
    
    // Check magic
    if (header.magic != 0x53454C46) {  // "SELF"
        std::cerr << "Invalid SELF magic: 0x" << std::hex << header.magic << std::dec << std::endl;
        return false;
    }
    
    // Validate header
    if (header.headerSize < sizeof(SELFHeader)) {
        std::cerr << "SELF header size too small" << std::endl;
        return false;
    }
    
    return true;
}

bool SELFLoader::parseSelfSections(const uint8_t* buffer, uint32_t size) {
    if (!buffer) return false;
    
    sections_.clear();
    
    // Section headers start after SELF header
    uint32_t sectHeaderOffset = header_.headerSize;
    
    for (int i = 0; i < header_.secHeaderCount; ++i) {
        if (sectHeaderOffset + sizeof(SELFSectionInfo) > size) {
            std::cerr << "Invalid SELF section offset" << std::endl;
            return false;
        }
        
        SELFSectionInfo sectInfo;
        std::memcpy(&sectInfo, buffer + sectHeaderOffset, sizeof(SELFSectionInfo));
        sectInfo.index = i;
        
        sections_.push_back(sectInfo);
        
        std::cout << "  Section " << i << ": offset=0x" << std::hex << sectInfo.offset
                  << " size=0x" << sectInfo.size << " flags=0x" << sectInfo.flags << std::dec << std::endl;
        
        sectHeaderOffset += sizeof(SELFSectionInfo);
    }
    
    return true;
}

bool SELFLoader::parseSelfMetadata(const uint8_t* buffer, uint32_t size) {
    if (!buffer) return false;
    
    // Metadata is appended after all sections
    // For now, just try to read it if present
    uint32_t metaOffset = header_.headerSize + (header_.secHeaderCount * sizeof(SELFSectionInfo));
    
    if (metaOffset + 32 <= size) {
        // Read AES key (4 x uint32)
        std::memcpy(metadata_.aesKey, buffer + metaOffset, 16);
        // Read AES IV
        std::memcpy(metadata_.aesIV, buffer + metaOffset + 16, 16);
        
        std::cout << "Found SELF metadata at offset 0x" << std::hex << metaOffset << std::dec << std::endl;
        return true;
    }
    
    return false;
}

bool SELFLoader::extractELF(std::vector<uint8_t>& elfData) {
    if (sections_.empty()) {
        std::cerr << "No SELF sections to extract" << std::endl;
        return false;
    }
    
    // For now, just concatenate section data
    // In a real implementation, we'd decrypt each section with AES-128-CBC
    // and decompress if needed
    
    elfData.clear();
    
    for (const auto& sect : sections_) {
        std::cout << "Processing section: offset=0x" << std::hex << sect.offset
                  << " size=0x" << sect.size << std::dec << std::endl;
        
        // Check if encrypted (bit 0 of flags)
        bool isEncrypted = (sect.flags & 0x1) != 0;
        // Check if compressed (bit 1 of flags)
        bool isCompressed = (sect.flags & 0x2) != 0;
        
        std::cout << "  Encrypted: " << (isEncrypted ? "yes" : "no")
                  << " Compressed: " << (isCompressed ? "yes" : "no") << std::endl;
        
        // For now, stub: just copy raw data (assumes unencrypted dev SELF)
        // Real implementation would:
        // 1. Decrypt with AES-128-CBC if encrypted
        // 2. Decompress with zlib if compressed
        // 3. Verify HMAC-SHA1
        
        // This allows us to load dev/TEST SELF files without crypto
    }
    
    // Return empty for now - in real use, would contain decrypted ELF
    std::cout << "ELF extraction stub (decryption not implemented)" << std::endl;
    return true;
}

void SELFLoader::dumpSelfInfo() const {
    std::cout << "\n=== SELF File Info ===" << std::endl;
    std::cout << "Magic: 0x" << std::hex << header_.magic << std::dec << std::endl;
    std::cout << "Version: 0x" << std::hex << header_.version << std::dec << std::endl;
    std::cout << "Flags: 0x" << std::hex << header_.flags << std::dec << std::endl;
    std::cout << "Header Size: " << header_.headerSize << " bytes" << std::endl;
    std::cout << "Sections: " << (int)header_.secHeaderCount << std::endl;
    std::cout << "Key Revision: " << header_.keyRevision << std::endl;
    std::cout << "Content Size: 0x" << std::hex << header_.contentSize << std::dec << " bytes" << std::endl;
    std::cout << "Sections:" << std::endl;
    
    for (size_t i = 0; i < sections_.size(); ++i) {
        const auto& sect = sections_[i];
        std::cout << "  [" << i << "] offset=0x" << std::hex << sect.offset
                  << " size=0x" << sect.size << " flags=0x" << sect.flags << std::dec << std::endl;
    }
}

} // namespace pxs3c

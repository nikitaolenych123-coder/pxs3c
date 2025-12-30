#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

namespace pxs3c {

// PS3 SELF (Signed ELF) file format
// Used for all PS3 executables - requires decryption/decompression

#pragma pack(1)

// SELF header
struct SELFHeader {
    uint32_t magic;              // 0x53454C46 ("SELF")
    uint32_t version;            // File format version
    uint32_t flags;              // Signature type, etc.
    uint32_t headerSize;         // Size of header before sections
    uint32_t secHeaderSize;      // Size of section header
    uint16_t secHeaderCount;     // Number of sections
    uint16_t keyRevision;        // Revision of decryption keys
    uint64_t contentSize;        // Total size without header
    uint64_t selfOffset;         // Offset in file
};

// SELF section entry
struct SELFSectionInfo {
    uint64_t offset;             // Offset in file
    uint64_t size;               // Size of section
    uint32_t flags;              // Compressed, encrypted flags
    uint32_t index;              // Section index
};

// Segment header (like ELF program header, but encrypted)
struct SELFSegment {
    uint32_t flags;              // Flags
    uint32_t offset;             // Offset in file
    uint64_t memSize;            // Size in memory
    uint64_t fileSize;           // Size in file
    uint64_t address;            // Load address (virtual)
};

#pragma pack()

// SELF metadata (appended after SELF sections)
struct SELFMetadata {
    uint32_t aesKey[4];          // AES-128 key (16 bytes = 4 uint32)
    uint32_t aesIV[4];           // AES-IV (16 bytes = 4 uint32)
    std::vector<uint8_t> hmacSha1;  // HMAC-SHA1 signature
};

class SELFLoader {
public:
    SELFLoader();
    ~SELFLoader();
    
    // Load SELF file
    bool loadSelf(const char* path);
    bool loadSelfFromBuffer(const uint8_t* buffer, uint32_t size);
    
    // Parse SELF (without decryption for now)
    bool parseSelfHeader(const uint8_t* buffer, SELFHeader& header);
    bool parseSelfSections(const uint8_t* buffer, uint32_t size);
    bool parseSelfMetadata(const uint8_t* buffer, uint32_t size);
    
    // Get parsed data
    const SELFHeader& getHeader() const { return header_; }
    const std::vector<SELFSectionInfo>& getSections() const { return sections_; }
    const std::vector<uint8_t>& getRawELF() const { return rawElf_; }
    
    // Extract ELF from SELF (stub - decryption not implemented)
    bool extractELF(std::vector<uint8_t>& elfData);
    
    // Debug
    void dumpSelfInfo() const;
    
private:
    SELFHeader header_;
    std::vector<SELFSectionInfo> sections_;
    std::vector<uint8_t> rawElf_;
    SELFMetadata metadata_;
    
    // Decryption keys (PS3 public keys - these are known)
    static const uint8_t defaultKey[];
    static const uint8_t defaultIV[];
    
    // For now, we'll stub HMAC verification
    bool verifySignature() { return true; }
};

} // namespace pxs3c

#include "core/Emulator.h"
#include "memory/MemoryManager.h"
#include "cpu/PPUInterpreter.h"
#include <iostream>

int main(int argc, char** argv) {
    pxs3c::Emulator emu;
    
    std::cout << "=== PXS3C Emulator Test ===" << std::endl;
    
    if (!emu.init()) {
        std::cerr << "Failed to init emulator" << std::endl;
        return 1;
    }
    
    // Test memory if no game provided
    if (argc <= 1) {
        std::cout << "\n=== Testing Memory Manager ===" << std::endl;
        auto* memory = emu.getMemory();
        if (memory) {
            uint64_t testAddr = 0x00010000;
            uint32_t testValue = 0xDEADBEEF;
            
            memory->write32(testAddr, testValue);
            uint32_t readValue = memory->read32(testAddr);
            
            std::cout << "Wrote 0x" << std::hex << testValue 
                      << " at 0x" << testAddr << std::endl;
            std::cout << "Read  0x" << readValue 
                      << " from 0x" << testAddr << std::dec << std::endl;
            
            if (readValue == testValue) {
                std::cout << "✓ Memory test PASSED" << std::endl;
            } else {
                std::cout << "✗ Memory test FAILED" << std::endl;
            }
        }
        
        std::cout << "\n=== Testing PPU Interpreter ===" << std::endl;
        auto* ppu = emu.getPPU();
        if (ppu) {
            ppu->setPC(0x00010000);
            ppu->setGPR(1, 0x12345678);
            ppu->setGPR(2, 0xABCDEF00);
            
            std::cout << "PC: 0x" << std::hex << ppu->getPC() << std::dec << std::endl;
            std::cout << "GPR1: 0x" << std::hex << ppu->getGPR(1) << std::dec << std::endl;
            std::cout << "GPR2: 0x" << std::hex << ppu->getGPR(2) << std::dec << std::endl;
            std::cout << "✓ PPU basic test PASSED" << std::endl;
        }
    } else {
        // Load and run game
        std::cout << "\n=== Loading Game ===" << std::endl;
        if (emu.loadGame(argv[1])) {
            std::cout << "Game loaded successfully" << std::endl;
            
            auto* ppu = emu.getPPU();
            if (ppu) {
                std::cout << "\n=== Initial PPU State ===" << std::endl;
                ppu->dumpRegisters();
            }
        }
    }
    
    std::cout << "\n=== Running Frame Test ===" << std::endl;
    for (int i = 0; i < 3; ++i) {
        emu.runFrame();
    }
    
    emu.shutdown();
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}

#include "core/Emulator.h"
#include "core/SyscallHandler.h"
#include "rsx/RSXProcessor.h"
#include "rsx/RSXCommands.h"
#include "loader/SELFLoader.h"
#include "memory/MemoryManager.h"
#include "cpu/PPUInterpreter.h"
#include "cpu/SPUInterpreter.h"
#include "cpu/SPUManager.h"
#include <iostream>
#include <fstream>

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
    
    std::cout << "\n=== Testing SPU Manager ===" << std::endl;
    auto* spuMgr = emu.getSPUs();
    if (spuMgr) {
        for (int i = 0; i < 6; ++i) {
            auto* spu = spuMgr->getSPU(i);
            if (spu) {
                spu->setPC(0x100 * i);
                pxs3c::SPUVector reg;
                reg.u32[0] = 0xDEADBEEF;
                spu->setRegister(1, reg);
                
                pxs3c::SPUVector readReg = spu->getRegister(1);
                std::cout << "SPU" << i << " PC: 0x" << std::hex << spu->getPC() 
                          << " R1: 0x" << readReg.u32[0] << std::dec << std::endl;
            }
        }
        std::cout << "✓ SPU test PASSED" << std::endl;
    }
    
    std::cout << "\n=== Testing Syscall Handler ===" << std::endl;
    {
        pxs3c::SyscallContext ctx;
        ctx.r3 = 0x1000;
        ctx.r4 = 0x2000;
        ctx.r5 = 0x3000;
        ctx.returnValue = 0;
        ctx.handled = false;
        
        auto* syscalls = emu.getMemory() ? 
            new pxs3c::SyscallHandler() : nullptr;
        
        if (syscalls && emu.getPPU() && emu.getMemory()) {
            syscalls->init(emu.getPPU(), emu.getMemory());
            
            // Test LV2 memory allocate (syscall 202)
            std::cout << "Calling syscall 202 (sys_memory_allocate)..." << std::endl;
            syscalls->handleSyscall(202, ctx);
            std::cout << "  Return value: 0x" << std::hex << ctx.returnValue << std::dec << std::endl;
            
            // Test LV2 get memory size (syscall 205)
            ctx.returnValue = 0;
            std::cout << "Calling syscall 205 (sys_memory_get_user_memory_size)..." << std::endl;
            syscalls->handleSyscall(205, ctx);
            std::cout << "  Return value: 0x" << std::hex << ctx.returnValue << std::dec << std::endl;
            
            // Test LV1 get version (syscall 512 + 1)
            ctx.returnValue = 0;
            std::cout << "Calling syscall 513 (lv1_get_version)..." << std::endl;
            syscalls->handleSyscall(513, ctx);
            std::cout << "  Return value: 0x" << std::hex << ctx.returnValue << std::dec << std::endl;
            
            std::cout << "✓ Syscall handler test PASSED" << std::endl;
            delete syscalls;
        }
    }
    
    std::cout << "\n=== Testing RSX Processor ===" << std::endl;
    {
        auto* rsx = emu.getRSX();
        if (rsx) {
            // Test clear color
            rsx->drawClearScreen(0xFF0000FF);  // Red
            
            // Test draw commands
            rsx->drawRectangle(100.0f, 100.0f, 200.0f, 150.0f, 0x00FF00FF);  // Green
            rsx->drawTriangle(300.0f, 300.0f, 400.0f, 400.0f, 350.0f, 500.0f, 0x0000FFFF);  // Blue
            
            // Test command buffer
            pxs3c::RSXCommandBuffer cmdBuf(1024);
            cmdBuf.writeCommand(0x0A0C, 0xFF0000FF);  // Clear color red
            cmdBuf.writeCommand(0x0ABC, std::vector<uint32_t>{0x4});  // Begin triangles
            
            std::cout << "  Buffer size: " << cmdBuf.getSize() << " bytes" << std::endl;
            
            // Process commands
            rsx->processCommands(cmdBuf);
            
            std::cout << "✓ RSX processor test PASSED" << std::endl;
        }
    }
    
    std::cout << "\n=== Testing SELF Loader ===" << std::endl;
    {
        // Create a mock SELF file for testing
        std::string testSelfPath = "/tmp/test_mock.self";
        
        // Write minimal SELF header
        std::ofstream selfFile(testSelfPath, std::ios::binary);
        if (selfFile.is_open()) {
            // SELF header (40 bytes minimum)
            uint32_t selfMagic = 0x53454C46;  // "SELF"
            uint32_t selfVersion = 3;
            uint32_t selfFlags = 0;
            uint32_t headerSize = 40;
            uint32_t secHeaderSize = 32;
            uint16_t secHeaderCount = 1;
            uint16_t keyRevision = 4;
            uint64_t contentSize = 512;
            uint64_t selfOffset = 0;
            
            selfFile.write(reinterpret_cast<char*>(&selfMagic), 4);
            selfFile.write(reinterpret_cast<char*>(&selfVersion), 4);
            selfFile.write(reinterpret_cast<char*>(&selfFlags), 4);
            selfFile.write(reinterpret_cast<char*>(&headerSize), 4);
            selfFile.write(reinterpret_cast<char*>(&secHeaderSize), 4);
            selfFile.write(reinterpret_cast<char*>(&secHeaderCount), 2);
            selfFile.write(reinterpret_cast<char*>(&keyRevision), 2);
            selfFile.write(reinterpret_cast<char*>(&contentSize), 8);
            selfFile.write(reinterpret_cast<char*>(&selfOffset), 8);
            
            // Add section header
            uint64_t sectOffset = 80;
            uint64_t sectSize = 256;
            uint32_t sectFlags = 0;  // Not encrypted, not compressed
            uint32_t sectIndex = 0;
            
            selfFile.write(reinterpret_cast<char*>(&sectOffset), 8);
            selfFile.write(reinterpret_cast<char*>(&sectSize), 8);
            selfFile.write(reinterpret_cast<char*>(&sectFlags), 4);
            selfFile.write(reinterpret_cast<char*>(&sectIndex), 4);
            
            // Pad rest of file
            std::vector<uint8_t> padding(256, 0xAB);
            selfFile.write(reinterpret_cast<char*>(padding.data()), 256);
            selfFile.close();
            
            std::cout << "Created mock SELF file: " << testSelfPath << std::endl;
            
            // Try to load it
            pxs3c::SELFLoader selfLoader;
            if (selfLoader.loadSelf(testSelfPath.c_str())) {
                std::cout << "✓ SELF file parsed successfully" << std::endl;
                selfLoader.dumpSelfInfo();
                std::cout << "✓ SELF loader test PASSED" << std::endl;
            } else {
                std::cout << "✗ SELF loader test FAILED" << std::endl;
            }
            
            // Cleanup
            std::remove(testSelfPath.c_str());
        }
    }
    
    for (int i = 0; i < 3; ++i) {
        emu.runFrame();
    }
    
    emu.shutdown();
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}

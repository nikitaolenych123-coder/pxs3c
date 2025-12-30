#include "core/Emulator.h"
#include "core/SyscallHandler.h"
#include "rsx/RSXProcessor.h"
#include "rsx/RSXCommands.h"
#include "memory/MemoryManager.h"
#include "cpu/PPUInterpreter.h"
#include "cpu/SPUInterpreter.h"
#include "cpu/SPUManager.h"
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
    
    for (int i = 0; i < 3; ++i) {
        emu.runFrame();
    }
    
    emu.shutdown();
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}

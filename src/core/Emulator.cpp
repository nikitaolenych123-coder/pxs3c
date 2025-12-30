#include "core/Emulator.h"
#include "core/SyscallHandler.h"
#include "rsx/VulkanRenderer.h"
#include "rsx/RSXProcessor.h"
#include "core/FramePacer.h"
#include "cpu/engines/Rpcs3Bridge.h"
#include "memory/MemoryManager.h"
#include "loader/ElfLoader.h"
#include "cpu/PPUInterpreter.h"
#include "cpu/SPUManager.h"
#include <cstring>
#include <iostream>

namespace pxs3c {

Emulator::Emulator() = default;
Emulator::~Emulator() = default;

bool Emulator::init() {
    // Initialize memory manager
    memory_ = std::make_unique<MemoryManager>();
    if (!memory_->init()) {
        std::cerr << "Memory manager init failed" << std::endl;
        return false;
    }
    
    // Initialize PPU interpreter
    ppu_ = std::make_unique<PPUInterpreter>();
    
    // Initialize syscall handler first (before PPU init)
    syscallHandler_ = std::make_unique<SyscallHandler>();
    if (!syscallHandler_->init(ppu_.get(), memory_.get())) {
        std::cerr << "Syscall handler init failed" << std::endl;
        return false;
    }
    
    if (!ppu_->init(memory_.get(), syscallHandler_.get())) {
        std::cerr << "PPU interpreter init failed" << std::endl;
        return false;
    }
    
    // Initialize SPU manager (6 cores)
    spuManager_ = std::make_unique<SPUManager>();
    // Convert unique_ptr to shared_ptr for SPU usage
    std::shared_ptr<MemoryManager> memShared(memory_.get(), [](void*) {});
    if (!spuManager_->init(memShared)) {
        std::cerr << "SPU manager init failed" << std::endl;
        return false;
    }
    
    // Initialize renderer
    renderer_ = std::make_unique<VulkanRenderer>();
    if (!renderer_->init()) {
        std::cerr << "Renderer init failed" << std::endl;
        return false;
    }
    
    // Initialize RSX processor (GPU command processor)
    rsx_ = std::make_unique<RSXProcessor>();
    if (!rsx_->init(renderer_.get())) {
        std::cerr << "RSX processor init failed" << std::endl;
        return false;
    }
    
    // Initialize frame pacer
    framePacer_ = std::make_unique<FramePacer>();
    framePacer_->setTargetFps(60);
    
    // Initialize ELF loader
    elfLoader_ = std::make_unique<ElfLoader>();
    
    std::cout << "Emulator initialized" << std::endl;
    memory_->dumpRegions();
    return true;
}

bool Emulator::loadGame(const char* path) {
    std::cout << "Loading game: " << path << std::endl;
    
    // Check file extension
    std::string pathStr(path);
    auto endsWith = [&](const char* suffix) {
        const size_t n = std::strlen(suffix);
        return pathStr.size() >= n && pathStr.compare(pathStr.size() - n, n, suffix) == 0;
    };

    if (endsWith(".pkg") || endsWith(".PKG") || endsWith(".iso") || endsWith(".ISO")) {
        std::cerr << "Unsupported game format (PKG/ISO not implemented yet): " << pathStr << std::endl;
        return false;
    }

    bool isSelf = pathStr.size() > 5 && 
                  pathStr.substr(pathStr.size() - 5) == ".self";
    
    if (isSelf) {
        // Try to load SELF file
        if (elfLoader_ && elfLoader_->loadSelf(path, memory_.get())) {
            std::cout << "SELF file loaded" << std::endl;
            return true;
        }
        std::cerr << "Failed to load SELF file" << std::endl;
        return false;
    }
    
    // Try to load ELF file
    if (elfLoader_ && elfLoader_->load(path, memory_.get())) {
        uint64_t entry = elfLoader_->getEntryPoint();
        std::cout << "ELF loaded, entry point: 0x" << std::hex << entry << std::dec << std::endl;
        
        // Set PPU entry point
        if (ppu_) {
            ppu_->setPC(entry);
            std::cout << "PPU ready to execute from 0x" << std::hex << entry << std::dec << std::endl;
        }
        
        return true;
    }
    
    // Fallback to RPCS3 bridge if available
    if (!initializeEngine()) {
        std::cerr << "No engine available (RPCS3 bridge not found)" << std::endl;
        return false;
    }
    bool ok = engine_->loadElf(path);
    if (!ok) std::cerr << "Engine failed to load ELF: " << path << std::endl;
    return ok;
}

void Emulator::runFrame() {
    // Execute PPU instructions (1000 per frame for ~60fps)
    if (ppu_) {
        ppu_->executeBlock(1000);
    }
    
    // Execute SPU instructions in parallel (6 cores)
    if (spuManager_) {
        spuManager_->executeAllSPUs(500);
    }
    
    // Fallback to engine if available
    if (engine_) {
        engine_->runFrame();
    }
    
    // Render frame
    renderer_->drawFrame();
}

void Emulator::shutdown() {
    renderer_.reset();
    std::cout << "Emulator shutdown" << std::endl;
}

#ifdef __ANDROID__
bool Emulator::attachAndroidWindow(ANativeWindow* window) {
    if (!renderer_) {
        renderer_ = std::make_unique<VulkanRenderer>();
        if (!renderer_->init()) {
            std::cerr << "Renderer init failed" << std::endl;
            return false;
        }
    }
    return renderer_->attachAndroidWindow(window);
}
#endif

void Emulator::setTargetFps(int fps) {
    if (!pacer_) pacer_ = std::make_unique<FramePacer>();
    pacer_->setTargetFps(fps);
}

int Emulator::tickFrameAndGetDelayMs() {
    if (!pacer_) pacer_ = std::make_unique<FramePacer>();
    pacer_->beginFrame();
    if (engine_) engine_->runFrame();
    renderer_->drawFrame();
    return pacer_->endFrameAndSuggestDelayMs();
}

void Emulator::setClearColor(float r, float g, float b) {
    if (renderer_) renderer_->setClearColor(r, g, b);
}

void Emulator::setVsync(bool enabled) {
#ifdef __ANDROID__
    if (renderer_) renderer_->setPresentModeAndroid(enabled ? 0 : 1);
#else
    (void)enabled;
#endif
}

bool Emulator::initializeEngine() {
    if (engine_) return true;
    // Try RPCS3 bridge first
    std::unique_ptr<Engine> candidate = std::make_unique<Rpcs3Bridge>();
    if (candidate->init()) {
        engine_ = std::move(candidate);
        return true;
    }
    return false;
}

} // namespace pxs3c

#pragma once
#include <memory>

namespace pxs3c {

class VulkanRenderer;
class Engine;
class FramePacer;
class MemoryManager;
class ElfLoader;
class PPUInterpreter;
class SPUManager;
class SyscallHandler;

class Emulator {
public:
    Emulator();
    ~Emulator();

    bool init();
    bool loadGame(const char* path);
    void runFrame();
    void shutdown();

#ifdef __ANDROID__
    bool attachAndroidWindow(struct ANativeWindow* window);
#endif

    void setTargetFps(int fps);
    int tickFrameAndGetDelayMs();

    // Config updates
    void setClearColor(float r, float g, float b);
    void setVsync(bool enabled); // maps to present mode on Android
    
    // Component access
    MemoryManager* getMemory() { return memory_.get(); }
    PPUInterpreter* getPPU() { return ppu_.get(); }
    SPUManager* getSPUs() { return spuManager_.get(); }
    
private:
    std::unique_ptr<VulkanRenderer> renderer_;
    std::unique_ptr<FramePacer> framePacer_;
    std::unique_ptr<MemoryManager> memory_;
    std::unique_ptr<ElfLoader> elfLoader_;
    std::unique_ptr<PPUInterpreter> ppu_;
    std::unique_ptr<SPUManager> spuManager_;
    std::unique_ptr<SyscallHandler> syscallHandler_;
    std::unique_ptr<class FramePacer> pacer_;
    std::unique_ptr<Engine> engine_;
    bool initializeEngine();
};

} // namespace pxs3c

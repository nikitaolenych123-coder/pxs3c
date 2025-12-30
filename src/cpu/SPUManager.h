#pragma once

#include "cpu/SPUInterpreter.h"
#include <array>
#include <memory>

namespace pxs3c {

class MemoryManager;

// SPU Manager - controls 6 SPU cores
class SPUManager {
public:
    SPUManager();
    ~SPUManager();

    bool init(std::shared_ptr<MemoryManager> mainMemory);
    void shutdown();
    
    // Access individual SPUs
    SPUInterpreter* getSPU(int id) {
        if (id < 0 || id >= 6) return nullptr;
        return spus_[id].get();
    }
    
    // Execute all SPUs in parallel (simulated)
    void executeAllSPUs(int maxInstructions = 1000);
    void executeAllSPUsParallel(int maxInstructions = 1000);
    
    // Status
    void dumpAllRegisters() const;
    
private:
    std::array<std::unique_ptr<SPUInterpreter>, 6> spus_;
};

} // namespace pxs3c

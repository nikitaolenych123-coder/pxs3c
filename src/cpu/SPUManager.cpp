#include "cpu/SPUManager.h"
#include "memory/MemoryManager.h"
#include <iostream>
#include <thread>

namespace pxs3c {

SPUManager::SPUManager() {
    for (int i = 0; i < 6; ++i) {
        spus_[i] = std::make_unique<SPUInterpreter>(i);
    }
}

SPUManager::~SPUManager() {
    shutdown();
}

bool SPUManager::init(std::shared_ptr<MemoryManager> mainMemory) {
    std::cout << "Initializing 6 SPU cores..." << std::endl;
    for (int i = 0; i < 6; ++i) {
        if (!spus_[i]->init(mainMemory)) {
            std::cerr << "Failed to initialize SPU" << i << std::endl;
            return false;
        }
    }
    std::cout << "All SPU cores initialized (256KB local store each)" << std::endl;
    return true;
}

void SPUManager::shutdown() {
    for (int i = 0; i < 6; ++i) {
        spus_[i].reset();
    }
}

void SPUManager::executeAllSPUs(int maxInstructions) {
    // Sequential execution (simplified)
    for (int i = 0; i < 6; ++i) {
        if (!spus_[i]->isHalted()) {
            spus_[i]->executeBlock(maxInstructions);
        }
    }
}

void SPUManager::executeAllSPUsParallel(int maxInstructions) {
    // Parallel execution using threads
    std::array<std::thread, 6> threads;
    
    for (int i = 0; i < 6; ++i) {
        threads[i] = std::thread([this, i, maxInstructions]() {
            if (!spus_[i]->isHalted()) {
                spus_[i]->executeBlock(maxInstructions);
            }
        });
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < 6; ++i) {
        threads[i].join();
    }
}

void SPUManager::dumpAllRegisters() const {
    for (int i = 0; i < 6; ++i) {
        std::cout << "\n";
        spus_[i]->dumpRegisters();
    }
}

} // namespace pxs3c

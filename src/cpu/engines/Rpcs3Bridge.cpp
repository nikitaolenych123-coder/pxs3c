#include "cpu/engines/Rpcs3Bridge.h"
#include <iostream>
#include <dlfcn.h>

namespace pxs3c {

Rpcs3Bridge::Rpcs3Bridge() = default;
Rpcs3Bridge::~Rpcs3Bridge() { shutdown(); }

bool Rpcs3Bridge::init() {
    // Try load shared lib; real name to be agreed upon
    const char* candidates[] = {
        "librpcs3_bridge.so",
        "librpcs3.so"
    };
    for (auto name : candidates) {
        handle_ = dlopen(name, RTLD_NOW);
        if (handle_) break;
    }
    if (!handle_) {
        lastError_ = dlerror() ? dlerror() : "dlopen failed";
        std::cerr << "RPCS3 bridge not available: " << lastError_ << std::endl;
        return false;
    }
    if (!resolveSymbols()) {
        std::cerr << "RPCS3 bridge: symbol resolution failed" << std::endl;
        return false;
    }
    if (fnInit_) {
        int rc = fnInit_();
        if (rc != 0) {
            std::cerr << "RPCS3 bridge init returned " << rc << std::endl;
            return false;
        }
    }
    std::cout << "RPCS3 bridge initialized" << std::endl;
    return true;
}

bool Rpcs3Bridge::loadElf(const char* path) {
    if (!fnLoadElf_) return false;
    int rc = fnLoadElf_(path);
    return rc == 0;
}

void Rpcs3Bridge::runFrame() {
    if (fnRunFrame_) fnRunFrame_();
}

void Rpcs3Bridge::shutdown() {
    if (fnShutdown_) fnShutdown_();
    fnInit_ = nullptr;
    fnLoadElf_ = nullptr;
    fnRunFrame_ = nullptr;
    fnShutdown_ = nullptr;
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
    }
}

bool Rpcs3Bridge::resolveSymbols() {
    fnInit_ = reinterpret_cast<FnInit>(dlsym(handle_, "rpcs3_init"));
    fnLoadElf_ = reinterpret_cast<FnLoadElf>(dlsym(handle_, "rpcs3_load_elf"));
    fnRunFrame_ = reinterpret_cast<FnRunFrame>(dlsym(handle_, "rpcs3_run_frame"));
    fnShutdown_ = reinterpret_cast<FnShutdown>(dlsym(handle_, "rpcs3_shutdown"));
    // Allow partial availability; runFrame required for usefulness
    return fnRunFrame_ != nullptr;
}

} // namespace pxs3c

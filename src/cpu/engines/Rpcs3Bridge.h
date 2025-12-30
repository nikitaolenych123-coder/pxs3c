#pragma once
#include <string>
#include <memory>
#include "cpu/Engine.h"

namespace pxs3c {

class Rpcs3Bridge : public Engine {
public:
    Rpcs3Bridge();
    ~Rpcs3Bridge() override;

    bool init() override;
    bool loadElf(const char* path) override;
    void runFrame() override;
    void shutdown() override;

private:
    void* handle_ = nullptr; // dlopen handle
    // Example resolved symbols (hypothetical C API)
    using FnInit = int (*)();
    using FnLoadElf = int (*)(const char*);
    using FnRunFrame = void (*)();
    using FnShutdown = void (*)();

    FnInit fnInit_ = nullptr;
    FnLoadElf fnLoadElf_ = nullptr;
    FnRunFrame fnRunFrame_ = nullptr;
    FnShutdown fnShutdown_ = nullptr;

    bool resolveSymbols();
    std::string lastError_;
};

} // namespace pxs3c

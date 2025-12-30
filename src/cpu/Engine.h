#pragma once

namespace pxs3c {

class Engine {
public:
    virtual ~Engine() = default;
    virtual bool init() = 0;
    virtual bool loadElf(const char* path) = 0;
    virtual void runFrame() = 0;
    virtual void shutdown() = 0;
};

} // namespace pxs3c

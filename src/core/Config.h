#pragma once

namespace pxs3c {

struct Config {
    int targetFps = 60;          // 60 або 30
    bool vsync = true;           // FIFO vsync
    float clearR = 0.03f;        // колір фону
    float clearG = 0.03f;
    float clearB = 0.08f;
    int resolutionScale = 100;   // % (поки не застосовується)
};

} // namespace pxs3c

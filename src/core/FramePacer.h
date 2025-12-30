#pragma once
#include <cstdint>

namespace pxs3c {

class FramePacer {
public:
    FramePacer();
    void setTargetFps(int fps);
    int getTargetFps() const;

    void beginFrame();
    // Returns suggested delay in milliseconds before next frame
    int endFrameAndSuggestDelayMs();

private:
    int targetFps_ = 60;
    uint64_t targetFrameNs_ = 16666667ULL; // ~16.67ms
    uint64_t lastBeginNs_ = 0ULL;
    double avgFrameNs_ = 16666667.0; // EMA of frame time
    void autoAdjust();
};

} // namespace pxs3c

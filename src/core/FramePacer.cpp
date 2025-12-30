#include "core/FramePacer.h"
#include <chrono>

namespace pxs3c {

static inline uint64_t nowNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

FramePacer::FramePacer() {}

void FramePacer::setTargetFps(int fps) {
    if (fps < 1) fps = 1;
    targetFps_ = fps;
    targetFrameNs_ = 1000000000ULL / static_cast<uint64_t>(targetFps_);
}

int FramePacer::getTargetFps() const { return targetFps_; }

void FramePacer::beginFrame() {
    lastBeginNs_ = nowNs();
}

int FramePacer::endFrameAndSuggestDelayMs() {
    uint64_t endNs = nowNs();
    uint64_t elapsed = endNs - lastBeginNs_;
    // EMA with alpha=0.1
    avgFrameNs_ = 0.9 * avgFrameNs_ + 0.1 * static_cast<double>(elapsed);
    autoAdjust();
    int64_t remaining = static_cast<int64_t>(targetFrameNs_) - static_cast<int64_t>(elapsed);
    if (remaining < 0) remaining = 0;
    // Convert to ms, clamp to reasonable range
    int delayMs = static_cast<int>((remaining + 999999) / 1000000); // ceil
    if (delayMs > 1000) delayMs = 1000;
    return delayMs;
}

void FramePacer::autoAdjust() {
    // If aiming 60 and avg > ~18ms, drop to 30
    if (targetFps_ >= 60 && avgFrameNs_ > 18000000.0) {
        setTargetFps(30);
        return;
    }
    // If aiming 30 and avg < ~20ms consistently, try restore 60
    if (targetFps_ == 30 && avgFrameNs_ < 20000000.0) {
        setTargetFps(60);
        return;
    }
}

} // namespace pxs3c

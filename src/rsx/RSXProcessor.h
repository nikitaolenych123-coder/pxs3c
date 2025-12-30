#pragma once

#include "rsx/RSXCommands.h"
#include <memory>
#include <cstdint>
#include <vector>

namespace pxs3c {

class VulkanRenderer;

// RSX Processor - translates PS3 RSX commands to Vulkan draw calls
class RSXProcessor {
public:
    RSXProcessor();
    ~RSXProcessor();
    
    bool init(VulkanRenderer* renderer);
    void shutdown();
    
    // Process command buffer
    void processCommands(RSXCommandBuffer& cmdBuffer);
    
    // Direct command submission
    void submitCommand(uint32_t method, uint32_t value);
    void submitCommand(uint32_t method, const std::vector<uint32_t>& values);
    
    // Draw operations
    void drawRectangle(float x, float y, float width, float height, uint32_t color);
    void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color);
    void drawClearScreen(uint32_t color);
    
    // Get state
    const RSXDrawState& getDrawState() const { return state_; }
    void setDrawState(const RSXDrawState& state) { state_ = state; }
    
private:
    VulkanRenderer* renderer_;
    RSXDrawState state_;
    RSXCommandBuffer cmdBuffer_;
    
    // Command handlers
    void handleClearColor(uint32_t value);
    void handleViewport(uint32_t method, uint32_t value);
    void handleScissor(uint32_t method, uint32_t value);
    void handleBlendFunc(uint32_t srcFactor, uint32_t dstFactor);
    void handleBlendEquation(uint32_t equation);
    void handleCullFace(uint32_t mode);
    void handlePrimitive(uint32_t mode, uint32_t count);
    void handleBeginEnd(uint32_t primitive);
    void handleWaitForIdle();
    void handleNotify(uint32_t value);
};

} // namespace pxs3c

#include "rsx/RSXProcessor.h"
#include "rsx/VulkanRenderer.h"
#include <iostream>
#include <cmath>

namespace pxs3c {

RSXProcessor::RSXProcessor()
    : renderer_(nullptr) {
    state_.width = 1920;
    state_.height = 1080;
    state_.blendSrcFactor = RSXBlendFactor::SRC_ALPHA;
    state_.blendDstFactor = RSXBlendFactor::ONE_MINUS_SRC_ALPHA;
    state_.blendEquation = RSXBlendEquation::ADD;
    state_.clearColor = 0x000000FF;
    state_.primitive = RSXPrimitive::TRIANGLES;
    state_.cullingEnabled = true;
    state_.depthTestEnabled = true;
}

RSXProcessor::~RSXProcessor() {}

bool RSXProcessor::init(VulkanRenderer* renderer) {
    if (!renderer) return false;
    renderer_ = renderer;
    std::cout << "RSX Processor initialized" << std::endl;
    return true;
}

void RSXProcessor::shutdown() {
    renderer_ = nullptr;
}

void RSXProcessor::processCommands(RSXCommandBuffer& cmdBuffer) {
    RSXCommand cmd;
    
    while (cmdBuffer.readCommand(cmd)) {
        std::cout << "RSX command: method=0x" << std::hex << cmd.method 
                  << " count=" << std::dec << cmd.count << std::endl;
        
        switch (cmd.method) {
            case NV30_CLEAR_COLOR:
                handleClearColor(cmd.data.empty() ? 0 : cmd.data[0]);
                break;
                
            case NV30_VIEWPORT_HORIZONTAL:
            case NV30_VIEWPORT_VERTICAL:
                if (!cmd.data.empty()) {
                    handleViewport(cmd.method, cmd.data[0]);
                }
                break;
                
            case NV30_SCISSOR_HORIZONTAL:
            case NV30_SCISSOR_VERTICAL:
                if (!cmd.data.empty()) {
                    handleScissor(cmd.method, cmd.data[0]);
                }
                break;
                
            case NV30_BLEND_FUNC:
                if (cmd.data.size() >= 2) {
                    handleBlendFunc(cmd.data[0], cmd.data[1]);
                }
                break;
                
            case NV30_BLEND_EQUATION:
                if (!cmd.data.empty()) {
                    handleBlendEquation(cmd.data[0]);
                }
                break;
                
            case NV30_CULL_FACE:
                if (!cmd.data.empty()) {
                    handleCullFace(cmd.data[0]);
                }
                break;
                
            case NV30_BEGIN_END:
                if (!cmd.data.empty()) {
                    handleBeginEnd(cmd.data[0]);
                }
                break;
                
            case NV30_WAIT_FOR_IDLE:
                handleWaitForIdle();
                break;
                
            case NV30_NOTIFY:
                if (!cmd.data.empty()) {
                    handleNotify(cmd.data[0]);
                }
                break;
                
            default:
                std::cout << "  Unhandled RSX method: 0x" << std::hex << cmd.method << std::dec << std::endl;
                break;
        }
    }
}

void RSXProcessor::submitCommand(uint32_t method, uint32_t value) {
    std::vector<uint32_t> data = {value};
    submitCommand(method, data);
}

void RSXProcessor::submitCommand(uint32_t method, const std::vector<uint32_t>& values) {
    cmdBuffer_.writeCommand(method, values);
    RSXCommand cmd;
    if (cmdBuffer_.readCommand(cmd)) {
        // Process immediately
        std::cout << "RSX submit: method=0x" << std::hex << method << std::dec << std::endl;
    }
}

void RSXProcessor::drawRectangle(float x, float y, float width, float height, uint32_t color) {
    std::cout << "RSX draw rectangle: (" << x << "," << y << ") " 
              << width << "x" << height << " color=0x" << std::hex << color << std::dec << std::endl;
    
    if (renderer_) {
        // For now, use renderer's simple rectangle drawing
        // In a real implementation, this would build vertex/index buffers
    }
}

void RSXProcessor::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color) {
    std::cout << "RSX draw triangle: (" << x1 << "," << y1 << ") ("
              << x2 << "," << y2 << ") (" << x3 << "," << y3 
              << ") color=0x" << std::hex << color << std::dec << std::endl;
    
    if (renderer_) {
        // Build triangle and submit to renderer
    }
}

void RSXProcessor::drawClearScreen(uint32_t color) {
    std::cout << "RSX clear screen: color=0x" << std::hex << color << std::dec << std::endl;
    if (renderer_) {
        float r = ((color >> 24) & 0xFF) / 255.0f;
        float g = ((color >> 16) & 0xFF) / 255.0f;
        float b = ((color >> 8) & 0xFF) / 255.0f;
        renderer_->setClearColor(r, g, b);
    }
}

void RSXProcessor::handleClearColor(uint32_t value) {
    state_.clearColor = value;
    std::cout << "  Set clear color: 0x" << std::hex << value << std::dec << std::endl;
    drawClearScreen(value);
}

void RSXProcessor::handleViewport(uint32_t method, uint32_t value) {
    std::cout << "  Set viewport (method=0x" << std::hex << method << "): 0x" << value << std::dec << std::endl;
}

void RSXProcessor::handleScissor(uint32_t method, uint32_t value) {
    std::cout << "  Set scissor (method=0x" << std::hex << method << "): 0x" << value << std::dec << std::endl;
}

void RSXProcessor::handleBlendFunc(uint32_t srcFactor, uint32_t dstFactor) {
    state_.blendSrcFactor = static_cast<RSXBlendFactor>(srcFactor);
    state_.blendDstFactor = static_cast<RSXBlendFactor>(dstFactor);
    std::cout << "  Set blend func: src=0x" << std::hex << srcFactor 
              << " dst=0x" << dstFactor << std::dec << std::endl;
}

void RSXProcessor::handleBlendEquation(uint32_t equation) {
    state_.blendEquation = static_cast<RSXBlendEquation>(equation);
    std::cout << "  Set blend equation: 0x" << std::hex << equation << std::dec << std::endl;
}

void RSXProcessor::handleCullFace(uint32_t mode) {
    state_.cullingEnabled = (mode != 0x0404);  // GL_FRONT_AND_BACK
    std::cout << "  Set cull face: " << (state_.cullingEnabled ? "enabled" : "disabled") << std::endl;
}

void RSXProcessor::handlePrimitive(uint32_t mode, uint32_t count) {
    state_.primitive = static_cast<RSXPrimitive>(mode);
    std::cout << "  Draw primitive type=" << mode << " count=" << count << std::endl;
}

void RSXProcessor::handleBeginEnd(uint32_t primitive) {
    std::cout << "  Begin/end primitive: " << primitive << std::endl;
}

void RSXProcessor::handleWaitForIdle() {
    std::cout << "  Wait for RSX idle" << std::endl;
}

void RSXProcessor::handleNotify(uint32_t value) {
    std::cout << "  RSX notify: 0x" << std::hex << value << std::dec << std::endl;
}

} // namespace pxs3c

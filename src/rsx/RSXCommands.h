#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

namespace pxs3c {

// RSX (Reality Synthesizer) GPU command definitions for PS3
// Commands are 32-bit words in a command buffer/FIFO

struct RSXCommand {
    uint32_t method;      // Method number (which GPU operation)
    uint32_t count;       // How many data values follow
    std::vector<uint32_t> data;  // Command data
};

// RSX Method IDs (subset)
enum RSXMethod : uint32_t {
    // Viewport/Scissor
    NV30_VIEWPORT_HORIZONTAL = 0x0A20,
    NV30_VIEWPORT_VERTICAL = 0x0A24,
    NV30_SCISSOR_HORIZONTAL = 0x0C90,
    NV30_SCISSOR_VERTICAL = 0x0C94,
    
    // Matrix operations
    NV30_MATRIX_MODE = 0x0D60,
    NV30_MATRIX_PUSH = 0x0D68,
    NV30_MATRIX_POP = 0x0D6C,
    NV30_MATRIX_DATA = 0x0D70,
    
    // Drawing
    NV30_BLEND_FUNC = 0x0B04,
    NV30_BLEND_EQUATION = 0x0B0C,
    NV30_CLEAR_COLOR = 0x0A0C,
    NV30_CLEAR_VALUE = 0x0A0C,
    
    // Rasterization
    NV30_CULL_FACE = 0x0B44,
    NV30_FRONT_FACE = 0x0B46,
    NV30_POLYGON_MODE = 0x0A20,
    
    // Texture/Sampling
    NV30_TEX_ADDR = 0x1400,
    NV30_TEX_WRAP_S = 0x1408,
    NV30_TEX_WRAP_T = 0x1409,
    NV30_TEX_FORMAT = 0x140C,
    NV30_TEX_FILTER = 0x140F,
    
    // Primitive drawing
    NV30_BEGIN_END = 0x0ABC,
    
    // Vertex data
    NV30_VERTEX_ARRAY_POINTER_X = 0x1700,
    NV30_VERTEX_ARRAY_POINTER_Y = 0x1704,
    NV30_VERTEX_ARRAY_POINTER_Z = 0x1708,
    NV30_VERTEX_ARRAY_POINTER_W = 0x170C,
    
    // Framebuffer
    NV30_SURFACE_FORMAT = 0x0A20,
    NV30_SURFACE_PITCH = 0x0A24,
    NV30_SURFACE_OFFSET_ZETA = 0x0A20,
    
    // Synchronization
    NV30_NOTIFY = 0x0104,
    NV30_WAIT_FOR_IDLE = 0x1DFC,
};

// RSX Primitive types
enum class RSXPrimitive : uint32_t {
    POINTS = 0,
    LINES = 1,
    LINE_LOOP = 2,
    LINE_STRIP = 3,
    TRIANGLES = 4,
    TRIANGLE_STRIP = 5,
    TRIANGLE_FAN = 6,
    QUADS = 7,
    QUAD_STRIP = 8,
};

// RSX Blend factors
enum class RSXBlendFactor : uint32_t {
    ZERO = 0,
    ONE = 1,
    SRC_COLOR = 0x0300,
    ONE_MINUS_SRC_COLOR = 0x0301,
    SRC_ALPHA = 0x0302,
    ONE_MINUS_SRC_ALPHA = 0x0303,
    DST_ALPHA = 0x0304,
    ONE_MINUS_DST_ALPHA = 0x0305,
    DST_COLOR = 0x0306,
    ONE_MINUS_DST_COLOR = 0x0307,
};

// RSX Blend equation
enum class RSXBlendEquation : uint32_t {
    ADD = 0x8006,
    SUBTRACT = 0x800A,
    REVERSE_SUBTRACT = 0x800B,
    MIN = 0x8007,
    MAX = 0x8008,
};

// Vertex data format
struct RSXVertexData {
    float x, y, z, w;
    float nx, ny, nz;
    float u, v;
    uint32_t color;  // RGBA
};

// Draw state
struct RSXDrawState {
    uint32_t width;
    uint32_t height;
    RSXBlendFactor blendSrcFactor;
    RSXBlendFactor blendDstFactor;
    RSXBlendEquation blendEquation;
    uint32_t clearColor;
    RSXPrimitive primitive;
    bool cullingEnabled;
    bool depthTestEnabled;
};

// Simple command buffer for RSX
class RSXCommandBuffer {
public:
    RSXCommandBuffer(uint32_t capacity = 65536);
    ~RSXCommandBuffer() = default;
    
    // Write command to buffer
    void writeCommand(uint32_t method, const std::vector<uint32_t>& data);
    void writeCommand(uint32_t method, uint32_t value);
    
    // Read commands
    bool readCommand(RSXCommand& cmd);
    
    // Peek next command without consuming
    bool peekCommand(RSXCommand& cmd) const;
    
    // Buffer management
    void clear();
    uint32_t getSize() const { return currentPos_; }
    bool isEmpty() const { return currentPos_ == 0; }
    
    // Direct access
    const uint8_t* getBuffer() const { return buffer_.data(); }
    
private:
    std::vector<uint8_t> buffer_;
    uint32_t capacity_ = 0;
    uint32_t currentPos_ = 0;
    uint32_t readPos_ = 0;
};

} // namespace pxs3c

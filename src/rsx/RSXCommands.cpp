#include "rsx/RSXCommands.h"
#include <iostream>
#include <cstring>

namespace pxs3c {

RSXCommandBuffer::RSXCommandBuffer(uint32_t capacity)
    : currentPos_(0), readPos_(0) {
    buffer_.resize(capacity, 0);
}

void RSXCommandBuffer::writeCommand(uint32_t method, const std::vector<uint32_t>& data) {
    // Write method header (upper 16 bits = method, lower 16 bits = count)
    if (currentPos_ + 4 + data.size() * 4 >= buffer_.size()) {
        std::cerr << "RSX command buffer overflow" << std::endl;
        return;
    }
    
    uint32_t count = data.size();
    uint32_t header = (method << 16) | count;
    std::memcpy(buffer_.data() + currentPos_, &header, 4);
    currentPos_ += 4;
    
    // Write data
    for (uint32_t value : data) {
        std::memcpy(buffer_.data() + currentPos_, &value, 4);
        currentPos_ += 4;
    }
}

void RSXCommandBuffer::writeCommand(uint32_t method, uint32_t value) {
    std::vector<uint32_t> data = {value};
    writeCommand(method, data);
}

bool RSXCommandBuffer::readCommand(RSXCommand& cmd) {
    if (readPos_ >= currentPos_) {
        return false;  // No more commands
    }
    
    // Read header
    uint32_t header;
    std::memcpy(&header, buffer_.data() + readPos_, 4);
    readPos_ += 4;
    
    cmd.method = header >> 16;
    cmd.count = header & 0xFFFF;
    cmd.data.clear();
    cmd.data.resize(cmd.count);
    
    // Read data
    for (uint32_t i = 0; i < cmd.count; ++i) {
        uint32_t value;
        std::memcpy(&value, buffer_.data() + readPos_, 4);
        readPos_ += 4;
        cmd.data[i] = value;
    }
    
    return true;
}

bool RSXCommandBuffer::peekCommand(RSXCommand& cmd) const {
    if (readPos_ >= currentPos_) {
        return false;
    }
    
    uint32_t header;
    std::memcpy(&header, buffer_.data() + readPos_, 4);
    
    cmd.method = header >> 16;
    cmd.count = header & 0xFFFF;
    cmd.data.clear();
    cmd.data.resize(cmd.count);
    
    // Peek data
    uint32_t pos = readPos_ + 4;
    for (uint32_t i = 0; i < cmd.count && pos < currentPos_; ++i) {
        uint32_t value;
        std::memcpy(&value, buffer_.data() + pos, 4);
        pos += 4;
        cmd.data[i] = value;
    }
    
    return true;
}

void RSXCommandBuffer::clear() {
    currentPos_ = 0;
    readPos_ = 0;
}

} // namespace pxs3c

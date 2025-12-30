#pragma once

#include "rsx/VulkanRenderer.h"
#include <vulkan/vulkan.h>

namespace PXS3C {

/**
 * @brief Optimized Vulkan 1.3 renderer for Qualcomm Adreno 735 GPU
 * Implements thin-layer architecture with zero-copy techniques
 */
class VulkanRendererAdreno : public VulkanRenderer {
public:
    VulkanRendererAdreno();
    virtual ~VulkanRendererAdreno();

    // Override base renderer methods with Adreno optimizations
    bool initializeAdreno(ANativeWindow* window);
    void enableDynamicRendering(bool enable);
    void enableGraphicsPipelineLibrary(bool enable);
    void enableAsyncCompute(bool enable);
    void enableFSRUpscaling(bool enable);

    // Adreno-specific optimizations
    void setFramePacer(uint32_t target_fps); // 30/60 FPS lock
    void enableThermalBypass(bool enable);   // Experimental throttle bypass
    
    // Frame rendering with async compute
    void drawFrameAsync() override;

private:
    // Vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features vulkan13_features_;
    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features_;
    VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT gpl_features_;
    
    // Adreno-specific state
    bool adreno_optimizations_enabled_;
    bool dynamic_rendering_enabled_;
    bool gpl_enabled_;
    bool async_compute_enabled_;
    bool fsr_enabled_;
    uint32_t target_fps_;
    
    // Async compute for lighting/effects
    VkQueue compute_queue_;
    VkCommandPool compute_command_pool_;
    VkCommandBuffer compute_command_buffer_;
    
    // Frame pacer
    uint64_t last_frame_time_;
    uint64_t frame_interval_ns_;
    
    void createAdrenoOptimizedPipeline();
    void setupAsyncComputePipeline();
    void waitForFrameInterval();
};

} // namespace PXS3C

#pragma once
#include <cstdint>
#include <vector>

// Forward declare Android native window to avoid including Android headers on non-Android builds
struct ANativeWindow;

namespace pxs3c {

class VulkanRenderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() = default;

    bool init();
    void drawFrame();

    // Android-specific: attach native window and initialize Vulkan surface
    bool attachAndroidWindow(ANativeWindow* window);
    bool resize(uint32_t width, uint32_t height);
    void setClearColor(float r, float g, float b);
    void setPresentModeAndroid(int mode); // 0=FIFO,1=MAILBOX,2=IMMEDIATE (Android)

private:
#ifdef __ANDROID__
    // Minimal Vulkan state for Android
    void resetAndroidState();
    bool createInstance();
    bool createSurface(ANativeWindow* window);
    bool pickPhysicalDevice();
    bool createDeviceAndQueues();
    bool createSwapchain();
    bool createRenderPass();
    bool createFramebuffers();
    bool createCommandPoolAndBuffers();
    bool createSyncObjects();
    void cleanupSwapchain();

    void* instance_ = nullptr;         // VkInstance
    void* surface_ = nullptr;          // VkSurfaceKHR
    void* physicalDevice_ = nullptr;   // VkPhysicalDevice
    void* device_ = nullptr;           // VkDevice
    uint32_t graphicsQueueFamily_ = 0;
    uint32_t presentQueueFamily_ = 0;
    void* graphicsQueue_ = nullptr;    // VkQueue
    void* presentQueue_ = nullptr;     // VkQueue

    void* swapchain_ = nullptr;        // VkSwapchainKHR
    std::vector<void*> imageViews_;    // VkImageView
    void* renderPass_ = nullptr;       // VkRenderPass
    std::vector<void*> framebuffers_;  // VkFramebuffer
    void* commandPool_ = nullptr;      // VkCommandPool
    std::vector<void*> commandBuffers_; // VkCommandBuffer
    void* imageAvailableSemaphore_ = nullptr; // VkSemaphore
    void* renderFinishedSemaphore_ = nullptr; // VkSemaphore
    void* inFlightFence_ = nullptr;    // VkFence
    uint32_t extentWidth_ = 0;
    uint32_t extentHeight_ = 0;

    // Config state
    float clearR_ = 0.03f;
    float clearG_ = 0.03f;
    float clearB_ = 0.08f;
    int presentMode_ = 0; // 0=FIFO(default)
#endif
};

} // namespace pxs3c

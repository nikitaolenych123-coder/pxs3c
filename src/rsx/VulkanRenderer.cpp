#include "rsx/VulkanRenderer.h"
#include <iostream>

#ifdef __ANDROID__
#include <android/native_window.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#endif

namespace pxs3c {

bool VulkanRenderer::init() {
    std::cout << "VulkanRenderer init (stub)" << std::endl;
    return true;
}

bool VulkanRenderer::attachAndroidWindow(ANativeWindow* window) {
#ifdef __ANDROID__
    if (!window) {
        std::cerr << "attachAndroidWindow: null window" << std::endl;
        return false;
    }
    resetAndroidState();
    if (!createInstance()) return false;
    if (!createSurface(window)) return false;
    if (!pickPhysicalDevice()) return false;
    if (!createDeviceAndQueues()) return false;
    if (!createSwapchain()) return false;
    if (!createRenderPass()) return false;
    if (!createFramebuffers()) return false;
    if (!createCommandPoolAndBuffers()) return false;
    if (!createSyncObjects()) return false;
    std::cout << "Android Vulkan init complete (Adreno-targeted, swapchain ready)" << std::endl;
    return true;
#else
    (void)window;
    std::cerr << "attachAndroidWindow called on non-Android build" << std::endl;
    return false;
#endif
}

void VulkanRenderer::drawFrame() {
    std::cout << "Drawing frame (stub)" << std::endl;
#ifdef __ANDROID__
    if (!device_ || !swapchain_ || commandBuffers_.empty()) return;
    VkDevice device = (VkDevice)device_;
    VkSemaphore imageAvail = (VkSemaphore)imageAvailableSemaphore_;
    VkSemaphore renderFin = (VkSemaphore)renderFinishedSemaphore_;
    VkFence inFlight = (VkFence)inFlightFence_;
    vkWaitForFences(device, 1, &inFlight, VK_TRUE, 1000000000ULL);
    vkResetFences(device, 1, &inFlight);

    uint32_t imageIndex = 0;
    VkResult acq = vkAcquireNextImageKHR(device, (VkSwapchainKHR)swapchain_, 1000000000ULL, imageAvail, VK_NULL_HANDLE, &imageIndex);
    if (acq != VK_SUCCESS) return;

    VkCommandBuffer cmd = (VkCommandBuffer)commandBuffers_[imageIndex];
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkClearValue clearColor{};
    clearColor.color.float32[0] = clearR_;
    clearColor.color.float32[1] = clearG_;
    clearColor.color.float32[2] = clearB_;
    clearColor.color.float32[3] = 1.0f;

    VkRenderPassBeginInfo rpbi{};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = (VkRenderPass)renderPass_;
    rpbi.framebuffer = (VkFramebuffer)framebuffers_[imageIndex];
    rpbi.renderArea.offset = {0, 0};
    rpbi.renderArea.extent = {extentWidth_, extentHeight_};
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &clearColor;
    vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &imageAvail;
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &renderFin;
    vkQueueSubmit((VkQueue)graphicsQueue_, 1, &si, inFlight);

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    VkSwapchainKHR sc = (VkSwapchainKHR)swapchain_;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &renderFin;
    pi.swapchainCount = 1;
    pi.pSwapchains = &sc;
    pi.pImageIndices = &imageIndex;
    vkQueuePresentKHR((VkQueue)presentQueue_, &pi);
#endif
}

#ifdef __ANDROID__
void VulkanRenderer::resetAndroidState() {
    instance_ = nullptr;
    surface_ = nullptr;
    physicalDevice_ = nullptr;
    device_ = nullptr;
    graphicsQueueFamily_ = 0;
    presentQueueFamily_ = 0;
    graphicsQueue_ = nullptr;
    presentQueue_ = nullptr;
    swapchain_ = nullptr;
    renderPass_ = nullptr;
    commandPool_ = nullptr;
    imageAvailableSemaphore_ = nullptr;
    renderFinishedSemaphore_ = nullptr;
    inFlightFence_ = nullptr;
    clearR_ = 0.03f;
    clearG_ = 0.03f;
    clearB_ = 0.08f;
    presentMode_ = 0; // FIFO
}

bool VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "pxs3c";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "pxs3c";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };
    VkInstanceCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo = &appInfo;
    ici.enabledExtensionCount = 2;
    ici.ppEnabledExtensionNames = extensions;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult res = vkCreateInstance(&ici, nullptr, &instance);
    if (res != VK_SUCCESS) {
        std::cerr << "vkCreateInstance failed: " << res << std::endl;
        return false;
    }
    instance_ = instance;
    return true;
}

bool VulkanRenderer::createSurface(ANativeWindow* window) {
    VkAndroidSurfaceCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    sci.window = window;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult res = vkCreateAndroidSurfaceKHR((VkInstance)instance_, &sci, nullptr, &surface);
    if (res != VK_SUCCESS) {
        std::cerr << "vkCreateAndroidSurfaceKHR failed: " << res << std::endl;
        return false;
    }
    surface_ = surface;
    return true;
}

bool VulkanRenderer::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices((VkInstance)instance_, &count, nullptr);
    if (count == 0) {
        std::cerr << "No Vulkan physical devices found" << std::endl;
        return false;
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices((VkInstance)instance_, &count, devices.data());

    VkPhysicalDevice chosen = VK_NULL_HANDLE;
    for (auto d : devices) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(d, &props);
        // Qualcomm/Adreno vendor ID commonly 0x5143
        if (props.vendorID == 0x5143) {
            chosen = d; break;
        }
    }
    if (chosen == VK_NULL_HANDLE) {
        chosen = devices[0];
    }
    physicalDevice_ = chosen;
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(chosen, &props);
    std::cout << "Selected GPU: vendorID=" << props.vendorID << " deviceID=" << props.deviceID << std::endl;
    return true;
}

bool VulkanRenderer::createDeviceAndQueues() {
    // Query queue families
    uint32_t qCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice)physicalDevice_, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> qProps(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice)physicalDevice_, &qCount, qProps.data());

    // Find graphics + present
    bool graphicsFound = false, presentFound = false;
    for (uint32_t i = 0; i < qCount; ++i) {
        if ((qProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !graphicsFound) {
            graphicsQueueFamily_ = i;
            graphicsFound = true;
        }
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR((VkPhysicalDevice)physicalDevice_, i, (VkSurfaceKHR)surface_, &presentSupport);
        if (presentSupport && !presentFound) {
            presentQueueFamily_ = i;
            presentFound = true;
        }
    }
    if (!graphicsFound || !presentFound) {
        std::cerr << "Required queue families not found" << std::endl;
        return false;
    }

    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> dqci;
    VkDeviceQueueCreateInfo gq{};
    gq.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    gq.queueFamilyIndex = graphicsQueueFamily_;
    gq.queueCount = 1;
    gq.pQueuePriorities = &priority;
    dqci.push_back(gq);
    if (presentQueueFamily_ != graphicsQueueFamily_) {
        VkDeviceQueueCreateInfo pq{};
        pq.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        pq.queueFamilyIndex = presentQueueFamily_;
        pq.queueCount = 1;
        pq.pQueuePriorities = &priority;
        dqci.push_back(pq);
    }

    // Optional Adreno extensions
    std::vector<const char*> devExts = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    // Try to enable QCOM-specific transform if available (orientation handling)
    // We'll query availability before device creation ideally via features2, but keeping stub simple.

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = static_cast<uint32_t>(dqci.size());
    dci.pQueueCreateInfos = dqci.data();
    dci.enabledExtensionCount = static_cast<uint32_t>(devExts.size());
    dci.ppEnabledExtensionNames = devExts.data();

    VkDevice device = VK_NULL_HANDLE;
    VkResult res = vkCreateDevice((VkPhysicalDevice)physicalDevice_, &dci, nullptr, &device);
    if (res != VK_SUCCESS) {
        std::cerr << "vkCreateDevice failed: " << res << std::endl;
        return false;
    }
    device_ = device;

    VkQueue gqHandle = VK_NULL_HANDLE;
    vkGetDeviceQueue((VkDevice)device_, graphicsQueueFamily_, 0, &gqHandle);
    graphicsQueue_ = gqHandle;
    VkQueue pqHandle = VK_NULL_HANDLE;
    vkGetDeviceQueue((VkDevice)device_, presentQueueFamily_, 0, &pqHandle);
    presentQueue_ = pqHandle;

    std::cout << "Device and queues created (graphics=" << graphicsQueueFamily_ << ", present=" << presentQueueFamily_ << ")" << std::endl;
    return true;
}

bool VulkanRenderer::createSwapchain() {
    VkPhysicalDevice phys = (VkPhysicalDevice)physicalDevice_;
    VkDevice device = (VkDevice)device_;
    VkSurfaceKHR surface = (VkSurfaceKHR)surface_;

    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);

    VkSurfaceFormatKHR fmt{};
    uint32_t fmtCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &fmtCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(fmtCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &fmtCount, formats.data());
    fmt = formats[0];
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_R8G8B8A8_UNORM) { fmt = f; break; }
    }

    // Select present mode based on config (0=FIFO,1=MAILBOX,2=IMMEDIATE) if supported
    VkPresentModeKHR present = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t pmCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &pmCount, nullptr);
    std::vector<VkPresentModeKHR> modes(pmCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &pmCount, modes.data());
    VkPresentModeKHR desired = VK_PRESENT_MODE_FIFO_KHR;
    if (presentMode_ == 1) desired = VK_PRESENT_MODE_MAILBOX_KHR;
    else if (presentMode_ == 2) desired = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (auto m : modes) {
        if (m == desired) { present = desired; break; }
    }

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == 0xFFFFFFFF) {
        extent.width = 1280;
        extent.height = 720;
    }
    extentWidth_ = extent.width;
    extentHeight_ = extent.height;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = surface;
    sci.minImageCount = imageCount;
    sci.imageFormat = fmt.format;
    sci.imageColorSpace = fmt.colorSpace;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t indices[2] = {graphicsQueueFamily_, presentQueueFamily_};
    if (graphicsQueueFamily_ != presentQueueFamily_) {
        sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices = indices;
    } else {
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = present;
    sci.clipped = VK_TRUE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkResult res = vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain);
    if (res != VK_SUCCESS) {
        std::cerr << "vkCreateSwapchainKHR failed: " << res << std::endl;
        return false;
    }
    swapchain_ = swapchain;

    uint32_t scImgCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &scImgCount, nullptr);
    std::vector<VkImage> images(scImgCount);
    vkGetSwapchainImagesKHR(device, swapchain, &scImgCount, images.data());

    imageViews_.clear();
    imageViews_.resize(scImgCount);
    for (uint32_t i = 0; i < scImgCount; ++i) {
        VkImageViewCreateInfo ivci{};
        ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.image = images[i];
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = fmt.format;
        ivci.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;
        VkImageView view = VK_NULL_HANDLE;
        vkCreateImageView(device, &ivci, nullptr, &view);
        imageViews_[i] = view;
    }
    return true;
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription color{};
    color.format = VK_FORMAT_R8G8B8A8_UNORM; // assume chosen
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &color;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;

    VkRenderPass rp = VK_NULL_HANDLE;
    VkResult res = vkCreateRenderPass((VkDevice)device_, &rpci, nullptr, &rp);
    if (res != VK_SUCCESS) {
        std::cerr << "vkCreateRenderPass failed: " << res << std::endl;
        return false;
    }
    renderPass_ = rp;
    return true;
}

bool VulkanRenderer::createFramebuffers() {
    framebuffers_.clear();
    framebuffers_.resize(imageViews_.size());
    for (size_t i = 0; i < imageViews_.size(); ++i) {
        VkImageView attachments[] = {(VkImageView)imageViews_[i]};
        VkFramebufferCreateInfo fbci{};
        fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass = (VkRenderPass)renderPass_;
        fbci.attachmentCount = 1;
        fbci.pAttachments = attachments;
        fbci.width = extentWidth_;
        fbci.height = extentHeight_;
        fbci.layers = 1;
        VkFramebuffer fb = VK_NULL_HANDLE;
        VkResult res = vkCreateFramebuffer((VkDevice)device_, &fbci, nullptr, &fb);
        if (res != VK_SUCCESS) {
            std::cerr << "vkCreateFramebuffer failed: " << res << std::endl;
            return false;
        }
        framebuffers_[i] = fb;
    }
    return true;
}

bool VulkanRenderer::createCommandPoolAndBuffers() {
    VkCommandPoolCreateInfo cpci{};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = graphicsQueueFamily_;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkResult res = vkCreateCommandPool((VkDevice)device_, &cpci, nullptr, &pool);
    if (res != VK_SUCCESS) {
        std::cerr << "vkCreateCommandPool failed: " << res << std::endl;
        return false;
    }
    commandPool_ = pool;

    commandBuffers_.clear();
    commandBuffers_.resize(framebuffers_.size());
    VkCommandBufferAllocateInfo cbai{};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = (VkCommandPool)commandPool_;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());
    std::vector<VkCommandBuffer> bufs(commandBuffers_.size());
    res = vkAllocateCommandBuffers((VkDevice)device_, &cbai, bufs.data());
    if (res != VK_SUCCESS) {
        std::cerr << "vkAllocateCommandBuffers failed: " << res << std::endl;
        return false;
    }
    for (size_t i = 0; i < bufs.size(); ++i) commandBuffers_[i] = bufs[i];
    return true;
}

bool VulkanRenderer::createSyncObjects() {
    VkSemaphoreCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore imageAvail = VK_NULL_HANDLE, renderFin = VK_NULL_HANDLE;
    vkCreateSemaphore((VkDevice)device_, &sci, nullptr, &imageAvail);
    vkCreateSemaphore((VkDevice)device_, &sci, nullptr, &renderFin);
    imageAvailableSemaphore_ = imageAvail;
    renderFinishedSemaphore_ = renderFin;

    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence((VkDevice)device_, &fci, nullptr, &fence);
    inFlightFence_ = fence;
    return true;
}

void VulkanRenderer::cleanupSwapchain() {
    VkDevice device = (VkDevice)device_;
    if (!device) return;
    // Destroy framebuffers
    for (auto fbPtr : framebuffers_) {
        VkFramebuffer fb = (VkFramebuffer)fbPtr;
        if (fb) vkDestroyFramebuffer(device, fb, nullptr);
    }
    framebuffers_.clear();
    // Destroy image views
    for (auto ivPtr : imageViews_) {
        VkImageView iv = (VkImageView)ivPtr;
        if (iv) vkDestroyImageView(device, iv, nullptr);
    }
    imageViews_.clear();
    // Free command buffers
    if (commandPool_ && !commandBuffers_.empty()) {
        std::vector<VkCommandBuffer> bufs;
        bufs.reserve(commandBuffers_.size());
        for (auto cbPtr : commandBuffers_) bufs.push_back((VkCommandBuffer)cbPtr);
        vkFreeCommandBuffers(device, (VkCommandPool)commandPool_, (uint32_t)bufs.size(), bufs.data());
    }
    commandBuffers_.clear();
    // Destroy swapchain
    if (swapchain_) {
        vkDestroySwapchainKHR(device, (VkSwapchainKHR)swapchain_, nullptr);
        swapchain_ = nullptr;
    }
}

bool VulkanRenderer::resize(uint32_t width, uint32_t height) {
#ifdef __ANDROID__
    if (!device_ || !surface_) return false;
    extentWidth_ = width;
    extentHeight_ = height;
    vkDeviceWaitIdle((VkDevice)device_);
    cleanupSwapchain();
    if (!createSwapchain()) return false;
    if (!createFramebuffers()) return false;
    if (!createCommandPoolAndBuffers()) return false;
    return true;
#else
    (void)width; (void)height;
    return false;
#endif
}

void VulkanRenderer::setClearColor(float r, float g, float b) {
#ifdef __ANDROID__
    clearR_ = r; clearG_ = g; clearB_ = b;
#else
    (void)r; (void)g; (void)b;
#endif
}

void VulkanRenderer::setPresentModeAndroid(int mode) {
#ifdef __ANDROID__
    presentMode_ = mode;
    if (device_) {
        vkDeviceWaitIdle((VkDevice)device_);
        cleanupSwapchain();
        createSwapchain();
        createFramebuffers();
        createCommandPoolAndBuffers();
    }
#else
    (void)mode;
#endif
}

} // namespace pxs3c

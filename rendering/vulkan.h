#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <map>
#include <compare>
#include <span>
#include <utility>
#include <iostream>
#include <cstring>

#include "framework/context.h"
#include "window/window.h"
#include "thread_pool/mutex.h"
#include "rendering/utils.h"
#include "utils/move_detector.h"

namespace pt {

namespace vulkan::detail {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool is_complete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };


    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
}

struct NewFrame {};

class CommandBufferHandle {
public:
    auto operator<=>(const CommandBufferHandle&) const = default;
private:
    CommandBufferHandle() = default;
    friend class VulkanRendering;
    double render_order;
    size_t idx;
};

struct NewCommandBufferHandle {
    using ResponseT = CommandBufferHandle;

    double render_order;
};

struct NewCommandPool {
    using ResponseT = VkCommandPool;
};

struct UpdateCommandBuffers {
    // whether there were previously buffers registered with this handle
    using ResponseT = bool; 

    // handle from NewCommandBufferHandle
    CommandBufferHandle handle;

    // one buffer per swapchain image. Must be in the same order as the
    // swapchain images in NewSwapChain. VulkanRendering does not
    // take ownership of these commandBuffers, it is still your responsibility
    // to clean them up.
    std::vector<VkCommandBuffer> commandBuffers;
};

struct TransferDataToBuffer {
    using ResponseT = void;

    std::span<const char> data;
    VkBuffer dst_buffer;
};

struct SwapChainInfo {
    std::span<VkImage> images;
    VkFormat imageFormat;
    VkExtent2D extent;
};

struct NewSwapChain {
    SwapChainInfo info;
    VkDevice device;
};

// emitted before rendering, another render will not
// be kicked off before all handlers have finished
// with PreRender
struct PreRender {};

struct GetVulkanPhysicalDevice {
    using ResponseT = VkPhysicalDevice;
};

struct GetVulkanDevice {
    using ResponseT = VkDevice;
};

struct GetSwapChainInfo {
    using ResponseT = SwapChainInfo;
};

class VulkanRendering {
public:
    template<IsContext C>
    VulkanRendering(C& ctx, size_t maxFramesInFlight): maxFramesInFlight(maxFramesInFlight) {
        window = ctx.request_sync(GetWindowPointer{});
        initVulkan();
        newSwapChain = true;
    }

    VulkanRendering(const VulkanRendering&) = delete;
    VulkanRendering(VulkanRendering&&) = default;

    VulkanRendering& operator=(const VulkanRendering&) = delete;
    VulkanRendering& operator=(VulkanRendering&&) = default;

    ~VulkanRendering();

    EVENT(NewFrame) {
        auto lock = co_await draw_mutex;

        if (newSwapChain) {
            if constexpr (ctx.template can_handle<NewSwapChain>()) {
                vkDeviceWaitIdle(device);
                co_await ctx.emit_await(NewSwapChain{
                    swapChainInfo(),
                    device,
                });
            }
            newSwapChain = false;
        }

        co_await ctx.emit_await(PreRender{});
        drawFrame();
    }

    EVENT(WindowResize) {
        framebufferResized = true;
        co_return;
    }

    REQUEST(NewCommandBufferHandle) {
        CommandBufferHandle handle;
        handle.idx = nextHandleIdx;
        handle.render_order = request.render_order;
        nextHandleIdx++;
        co_return handle;
    }

    REQUEST(UpdateCommandBuffers) {
        auto ret = commandBuffers.insert_or_assign(request.handle, request.commandBuffers);
        co_return !ret.second;
    }

    REQUEST(NewCommandPool) {
        co_return createCommandPool();
    }

    REQUEST(TransferDataToBuffer) {
        copyBuffer(request);
        co_return;
    }

    REQUEST(GetVulkanPhysicalDevice) {
        co_return physicalDevice;
    }

    REQUEST(GetVulkanDevice) {
        co_return device;
    }

    REQUEST(GetSwapChainInfo) {
        co_return swapChainInfo();
    }

private:
    void drawFrame();

    void initVulkan();
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createSyncObjects();

    void recreateSwapChain();
    SwapChainInfo swapChainInfo();

    VkCommandPool createCommandPool();
    void copyBuffer(const TransferDataToBuffer& request);

    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    vulkan::detail::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    vulkan::detail::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void cleanupSwapChain();
    void cleanup();

    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkCommandPool commandPool;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::map<CommandBufferHandle, std::vector<VkCommandBuffer>> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
    bool framebufferResized = false;
    bool newSwapChain = false;

    size_t maxFramesInFlight;
    size_t nextHandleIdx = 0;

    SingleThreadedMutex draw_mutex;
    MoveDetector move_detector;
};

}
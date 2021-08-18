#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <map>
#include <compare>
#include <span>
#include <utility>
#include <iostream>

#include "framework/context.h"
#include "window/window.h"
#include "thread_pool/mutex.h"

namespace pt {

namespace vulkan::detail {
    struct MoveDetector {
        MoveDetector() = default;
        MoveDetector(const MoveDetector&) = default;
        MoveDetector(MoveDetector&& o): moved(o.moved) {o.moved = true;}

        MoveDetector& operator=(const MoveDetector&) = default;
        MoveDetector& operator=(MoveDetector&& o) {
            if (this == &o) return *this;
            moved = o.moved;
            o.moved = true;
            return *this;
        }

        bool moved = false;
    };

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

struct NewSwapChain {
    std::span<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
};

class VulkanRendering {
public:
    VulkanRendering(size_t maxFramesInFlight): maxFramesInFlight(maxFramesInFlight) {}

    VulkanRendering(const VulkanRendering&) = delete;
    VulkanRendering(VulkanRendering&&) = default;

    VulkanRendering& operator=(const VulkanRendering&) = delete;
    VulkanRendering& operator=(VulkanRendering&&) = default;

    ~VulkanRendering();

    EVENT(NewWindow) {
        assert(!initialised);
        window = event.window;
        initVulkan();
        initialised = true;
        newSwapChain = true;
        co_return;
    }

    EVENT(NewFrame) {
        std::cout << __LINE__ << std::endl;
        if (!initialised) {
            co_return;
        }

        auto lock = co_await draw_mutex;
        if (newSwapChain) {
            if constexpr (ctx.template can_handle<NewSwapChain>()) {
                co_await ctx.emit_await(NewSwapChain{
                    .swapChainImages = std::span(swapChainImages),
                    .swapChainImageFormat = swapChainImageFormat,
                    .swapChainExtent = swapChainExtent,
                });
            }
            newSwapChain = false;
        }

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

    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    vulkan::detail::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    vulkan::detail::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void cleanupSwapChain();

    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;

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

    vulkan::detail::MoveDetector move_detector;
    size_t maxFramesInFlight;
    bool initialised = false;
    size_t nextHandleIdx = 0;

    SingleThreadedMutex draw_mutex;
};

}
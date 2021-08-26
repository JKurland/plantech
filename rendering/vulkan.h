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

struct NewSwapChain {
    std::span<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkDevice device;
};

struct DeleteSwapChain {
    VkDevice device;
};

struct VulkanInitialised {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
};

class VulkanRendering {
public:
    VulkanRendering(size_t maxFramesInFlight): maxFramesInFlight(maxFramesInFlight) {}

    VulkanRendering(const VulkanRendering&) = delete;
    VulkanRendering(VulkanRendering&&) = default;

    VulkanRendering& operator=(const VulkanRendering&) = delete;
    VulkanRendering& operator=(VulkanRendering&&) = default;


    EVENT(NewWindow) {
        assert(!initialised);
        window = event.window;
        initVulkan();
        initialised = true;
        newSwapChain = true;
        co_await ctx.emit_await(VulkanInitialised{
            .physicalDevice = physicalDevice,
            .device = device,
        });
        co_return;
    }

    EVENT(NewFrame) {
        auto lock = co_await draw_mutex;
        if (!initialised) {
            co_return;
        }

        if (newSwapChain) {
            if constexpr (ctx.template can_handle<NewSwapChain>()) {
                vkDeviceWaitIdle(device);
                co_await ctx.emit_await(NewSwapChain{
                    .swapChainImages = std::span(swapChainImages),
                    .swapChainImageFormat = swapChainImageFormat,
                    .swapChainExtent = swapChainExtent,
                    .device = device,
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

    EVENT(ClosingWindow) {
        if (event.window != window) co_return;
        auto lock = co_await draw_mutex;
        if (!initialised) co_return;

        co_await ctx.emit_await(DeleteSwapChain{device});
        cleanup();
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
        const VkDeviceSize bufferSize = request.data.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vkutils::createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            device,
            physicalDevice,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        std::memcpy(data, request.data.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        vkutils::copyBuffer(
            device,
            commandPool,
            graphicsQueue,
            stagingBuffer,
            request.dst_buffer,
            bufferSize
        );

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        co_return;
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

    VkCommandPool createCommandPool();

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
    bool initialised = false;
    size_t nextHandleIdx = 0;

    SingleThreadedMutex draw_mutex;
};

}
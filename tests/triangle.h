#pragma once

#include <vulkan/vulkan.h>

#include "rendering/vulkan.h"
#include "framework/context.h"

#include <vector>
#include <optional>
#include <span>
#include <cassert>
#include <iostream>

namespace pt {

class Triangle {
public:
    Triangle(double renderOrder): renderOrder(renderOrder) {}

    Triangle(const Triangle&) = delete;
    Triangle(Triangle&&) = default;
    Triangle& operator=(const Triangle&) = delete;
    Triangle& operator=(Triangle&&) = default;

    ~Triangle() {
        cleanup(device);
    }

    EVENT(NewSwapChain) {
        assert(!newSwapChainInProgress);
        newSwapChainInProgress = true;

        if (initialised) {
            cleanupSwapChain(event.device);
            initialised = false;
        }

        if (!commandBufferHandle.has_value()) {
            commandBufferHandle = co_await ctx(NewCommandBufferHandle{renderOrder});
        }

        if (!commandPool.has_value()) {
            commandPool = co_await ctx(NewCommandPool{});
        }

        device = event.device;
        createImageViews(event.info.images, event.info.imageFormat, event.device);
        createRenderPass(event.info.imageFormat, event.device);
        createGraphicsPipeline(event.info.extent, event.device);
        createFramebuffers(event.info.extent, event.device);
        createCommandBuffers(event.info.extent, event.device);

        auto req = UpdateCommandBuffers{
            *commandBufferHandle,
            commandBuffers,
        };
        co_await ctx(req);

        newSwapChainInProgress = false;
        initialised = true;
    }

private:
    void createImageViews(std::span<VkImage> swapChainImages, VkFormat swapChainImageFormat, VkDevice device);
    void createRenderPass(VkFormat swapChainImageFormat, VkDevice device);
    void createGraphicsPipeline(VkExtent2D swapChainExtent, VkDevice device);
    void createFramebuffers(VkExtent2D swapChainExtent, VkDevice device);
    void createCommandBuffers(VkExtent2D swapChainExtent, VkDevice device);
    void cleanupSwapChain(VkDevice device);
    void cleanup(VkDevice device);

    VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

    double renderOrder;

    VkDevice device;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::optional<VkCommandPool> commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::optional<CommandBufferHandle> commandBufferHandle;

    bool newSwapChainInProgress = false;
    bool initialised = false;
};

}

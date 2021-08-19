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


    EVENT(NewSwapChain) {
        assert(!newSwapChainInProgress);
        newSwapChainInProgress = true;
        if (!commandBufferHandle.has_value()) {
            commandBufferHandle = co_await ctx(NewCommandBufferHandle{renderOrder});
        }

        if (!commandPool.has_value()) {
            commandPool = co_await ctx(NewCommandPool{});
        }

        device = event.device;

        if (initialised) {
            cleanupSwapChain();
            initialised = false;
        }

        createImageViews(event.swapChainImages, event.swapChainImageFormat);
        createRenderPass(event.swapChainImageFormat);
        createGraphicsPipeline(event.swapChainExtent);
        createFramebuffers(event.swapChainExtent);
        createCommandBuffers(event.swapChainExtent);

        auto req = UpdateCommandBuffers{
            *commandBufferHandle,
            commandBuffers,
        };
        co_await ctx(req);

        newSwapChainInProgress = false;
        initialised = true;
    }

    EVENT(DeleteSwapChain) {
        assert(!newSwapChainInProgress);
        if (event.device != device) co_return;
        cleanup();
        co_return;
    }

private:
    void createImageViews(std::span<VkImage> swapChainImages, VkFormat swapChainImageFormat);
    void createRenderPass(VkFormat swapChainImageFormat);
    void createGraphicsPipeline(VkExtent2D swapChainExtent);
    void createFramebuffers(VkExtent2D swapChainExtent);
    void createCommandBuffers(VkExtent2D swapChainExtent);
    void cleanupSwapChain();
    void cleanup();

    VkShaderModule createShaderModule(const std::vector<char>& code);

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

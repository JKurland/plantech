#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "framework/context.h"
#include "rendering/vulkan.h"
#include "utils/move_detector.h"
#include "gui/gui.h"
#include "gui_rendering/walk_gui.h"
#include "gui_rendering/primitives.h"

#include <vector>
#include <optional>
#include <span>
#include <array>
#include <iostream>


namespace pt {

struct GetEventTargetForPixel {
    using ResponseT = EventTarget;
    std::uint32_t x;
    std::uint32_t y;
};

class GuiRenderer {
public:
    GuiRenderer(const GuiRenderer&) = delete;
    GuiRenderer(GuiRenderer&&) = default;
    GuiRenderer& operator=(const GuiRenderer&) = delete;
    GuiRenderer& operator=(GuiRenderer&&) = default;

    ~GuiRenderer();

    template<IsContext C>
    GuiRenderer(C& ctx): 
        swapChainInfo(ctx.request_sync(GetSwapChainInfo{})),
        device(ctx.request_sync(GetVulkanDevice{})),
        physicalDevice(ctx.request_sync(GetVulkanPhysicalDevice{})),
        commandPool(ctx.request_sync(NewCommandPool{})),
        commandBufferHandle(ctx.request_sync(NewCommandBufferHandle{}))
    {
        createVertexBuffer();
        ctx.request_sync(vertexBufferTransferRequest());

        createClickBuffer();
        createClickBufferDescriptor();

        initSwapChain();
    }


    EVENT(NewSwapChain) {
        assert(!newSwapChainInProgress);
        newSwapChainInProgress = true;

        cleanupSwapChain();

        swapChainInfo = event.info;
        initSwapChain();

        auto req = UpdateCommandBuffers{
            commandBufferHandle,
            commandBuffers,
        };
        co_await ctx(req);

        newSwapChainInProgress = false;
    }

    EVENT(PreRender) {
        VertexBufferBuilder vbBuilder(glm::uvec2(swapChainInfo.extent.width, swapChainInfo.extent.height));
        GuiVisitor visitor(vbBuilder);

        auto gui = co_await ctx(GetGui{});
        visitor.visit(gui);
        vertexBuffers = std::move(vbBuilder).build();

        vkDeviceWaitIdle(device);
        cleanupCommandBuffers();
        cleanupVertexBuffer();

        createVertexBuffer();
        createCommandBuffers();
        co_await ctx(vertexBufferTransferRequest());

        auto req = UpdateCommandBuffers{
            commandBufferHandle,
            commandBuffers,
        };
        co_await ctx(req);
        verticesChanged = false;
    }

    REQUEST(GetEventTargetForPixel) {
        vkDeviceWaitIdle(device);
        void* eventTargetIdxPtr = 0;

        const size_t offset = (request.x * swapChainInfo.extent.height + request.y) * clickBufferStride();
        VkResult result = vkMapMemory(device, clickBufferMemory, offset, sizeof(eventTargetIdxPtr), 0, &eventTargetIdxPtr);
        assert(result == VK_SUCCESS);

        uint32_t eventTargetIdx;
        memcpy(&eventTargetIdx, eventTargetIdxPtr, sizeof(eventTargetIdx));
        vkUnmapMemory(device, clickBufferMemory);

        assert(eventTargetIdx < vertexBuffers.eventTargets.size());
        co_return vertexBuffers.eventTargets[eventTargetIdx];
    }

private:
    void createVertexBuffer();
    TransferDataToBuffer vertexBufferTransferRequest();
    void createClickBuffer();
    void createClickBufferDescriptor();
    static size_t clickBufferStride();

    void initSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandBuffers();

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();

    void cleanupCommandBuffers();
    void cleanupDepthResources();
    void cleanupSwapChain();
    void cleanupVertexBuffer();
    void cleanupClickBuffer();
    void cleanupClickBufferDescriptor();
    void cleanup();


    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    SwapChainInfo swapChainInfo;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer clickBuffer = VK_NULL_HANDLE;
    VkDeviceMemory clickBufferMemory = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet clickBufferDescriptorSet = VK_NULL_HANDLE;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    CommandBufferHandle commandBufferHandle;

    bool newSwapChainInProgress = false;

    bool verticesChanged = false;
    VertexBuffers vertexBuffers;

    MoveDetector move_detector;
};

}
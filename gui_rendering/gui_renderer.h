#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "framework/context.h"
#include "rendering/vulkan.h"
#include "utils/move_detector.h"
#include "gui/gui.h"
#include "gui_rendering/walk_gui.h"

#include <vector>
#include <optional>
#include <span>
#include <array>


namespace pt {


class GuiRenderer {
public:
    GuiRenderer(const GuiRenderer&) = delete;
    GuiRenderer(GuiRenderer&&) = default;
    GuiRenderer& operator=(const GuiRenderer&) = delete;
    GuiRenderer& operator=(GuiRenderer&&) = default;

    ~GuiRenderer();

    template<IsContext C>
    GuiRenderer(C& ctx, double renderOrder): 
        swapChainInfo(ctx.request_sync(GetSwapChainInfo{})),
        device(ctx.request_sync(GetVulkanDevice{})),
        physicalDevice(ctx.request_sync(GetVulkanPhysicalDevice{})),
        commandPool(ctx.request_sync(NewCommandPool{})),
        commandBufferHandle(ctx.request_sync(NewCommandBufferHandle{renderOrder}))
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
        if (verticesChanged) {
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
    }

    EVENT(GuiUpdated) {
        VertexBufferBuilder vbBuilder(glm::uvec2(swapChainInfo.extent.width, swapChainInfo.extent.height));
        GuiVisitor visitor(vbBuilder);
        event.newGui.visitAll(visitor);
        vertices = vbBuilder.getTriangleVertices();
        verticesChanged = true;
        co_return;
    }

private:
    void createVertexBuffer();
    TransferDataToBuffer vertexBufferTransferRequest();
    void createClickBuffer();
    void createClickBufferDescriptor();

    void initSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandBuffers();

    void cleanupCommandBuffers();
    void cleanupSwapChain();
    void cleanupVertexBuffer();
    void cleanupClickBuffer();
    void cleanupClickBufferDescriptor();
    void cleanup();

    VkShaderModule createShaderModule(const std::vector<char>& code);

    SwapChainInfo swapChainInfo;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer clickBuffer = VK_NULL_HANDLE;
    VkDeviceMemory clickBufferMemory = VK_NULL_HANDLE;
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
    std::vector<TriangleVertex> vertices;

    MoveDetector move_detector;
};

}
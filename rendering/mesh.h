#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "framework/context.h"
#include "rendering/vulkan.h"
#include "utils/move_detector.h"

#include <vector>
#include <optional>
#include <span>
#include <array>


namespace pt {

struct Vertex {
    glm::vec2 pos;
    glm::vec3 colour;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

namespace mesh::detail {
    const std::vector<pt::Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };
}

class MeshRenderer {
public:
    MeshRenderer(const MeshRenderer&) = delete;
    MeshRenderer(MeshRenderer&&) = default;
    MeshRenderer& operator=(const MeshRenderer&) = delete;
    MeshRenderer& operator=(MeshRenderer&&) = default;

    ~MeshRenderer();

    template<IsContext C>
    MeshRenderer(C& ctx, double renderOrder): 
        swapChainInfo(ctx.request_sync(GetSwapChainInfo{})),
        device(ctx.request_sync(GetVulkanDevice{})),
        physicalDevice(ctx.request_sync(GetVulkanPhysicalDevice{})),
        commandPool(ctx.request_sync(NewCommandPool{})),
        commandBufferHandle(ctx.request_sync(NewCommandBufferHandle{renderOrder})),
        vertices(mesh::detail::vertices)
    {
        createVertexBuffer();
        ctx.request_sync(vertexBufferTransferRequest());

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

private:
    void createVertexBuffer();
    TransferDataToBuffer vertexBufferTransferRequest();

    void initSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandBuffers();

    void cleanupCommandBuffers();
    void cleanupSwapChain();
    void cleanupVertexBuffer();
    void cleanup();

    VkShaderModule createShaderModule(const std::vector<char>& code);

    SwapChainInfo swapChainInfo;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    CommandBufferHandle commandBufferHandle;

    bool newSwapChainInProgress = false;

    std::vector<pt::Vertex> vertices;

    MoveDetector move_detector;
};

}
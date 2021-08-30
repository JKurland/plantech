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

namespace mesh::detail {
    struct Vertex {
        glm::vec2 pos;
        glm::vec3 colour;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };

    const std::vector<pt::mesh::detail::Vertex> vertices = {
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
    MeshRenderer(C& ctx): 
        swapChainInfo(ctx.request_sync(GetSwapChainInfo{})),
        device(ctx.request_sync(GetVulkanDevice{})),
        commandPool(ctx.request_sync(NewCommandPool{})),
        commandBufferHandle(ctx.request_sync(NewCommandBufferHandle{renderOrder}))
    {
        createVertexBuffer(ctx.request_sync(GetVulkanPhysicalDevice{}));

        ctx.request_sync(TransferDataToBuffer{
            .data = std::span(
                reinterpret_cast<const char*>(mesh::detail::vertices.data()),
                mesh::detail::vertices.size() * sizeof(mesh::detail::vertices[0])
            ),
            .dst_buffer = vertexBuffer,
        });

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
    void createVertexBuffer(VkPhysicalDevice physicalDevice);

    void initSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandBuffers();
    void cleanupSwapChain();
    void cleanup();

    VkShaderModule createShaderModule(const std::vector<char>& code);

    double renderOrder = 0.0;

    SwapChainInfo swapChainInfo;
    VkDevice device;
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

    MoveDetector move_detector;
};

}
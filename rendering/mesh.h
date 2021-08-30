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
    MeshRenderer(C& ctx) {
        commandPool = ctx.request_sync(NewCommandPool{});

        device = ctx.request_sync(GetVulkanDevice{});
        createVertexBuffer(ctx.request_sync(GetVulkanPhysicalDevice{}));

        ctx.request_sync(TransferDataToBuffer{
            .data = std::span(
                reinterpret_cast<const char*>(mesh::detail::vertices.data()),
                mesh::detail::vertices.size() * sizeof(mesh::detail::vertices[0])
            ),
            .dst_buffer = vertexBuffer,
        });
    }

    EVENT(NewSwapChain) {
        assert(!newSwapChainInProgress);
        newSwapChainInProgress = true;


        if (swapChainInitialised) {
            cleanupSwapChain();
            swapChainInitialised = false;
        }

        if (!commandBufferHandle.has_value()) {
            commandBufferHandle = co_await ctx(NewCommandBufferHandle{renderOrder});
        }

        createImageViews(event.info.images, event.info.imageFormat);
        createRenderPass(event.info.imageFormat);
        createGraphicsPipeline(event.info.extent);
        createFramebuffers(event.info.extent);
        createCommandBuffers(event.info.extent);

        auto req = UpdateCommandBuffers{
            *commandBufferHandle,
            commandBuffers,
        };
        co_await ctx(req);

        newSwapChainInProgress = false;
        swapChainInitialised = true;
    }

private:
    void createVertexBuffer(VkPhysicalDevice physicalDevice);

    void createImageViews(std::span<VkImage> swapChainImages, VkFormat swapChainImageFormat);
    void createRenderPass(VkFormat swapChainImageFormat);
    void createGraphicsPipeline(VkExtent2D swapChainExtent);
    void createFramebuffers(VkExtent2D swapChainExtent);
    void createCommandBuffers(VkExtent2D swapChainExtent);
    void cleanupSwapChain();
    void cleanup();

    VkShaderModule createShaderModule(const std::vector<char>& code);

    double renderOrder = 0.0;

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

    std::optional<CommandBufferHandle> commandBufferHandle;

    bool newSwapChainInProgress = false;
    bool swapChainInitialised = false;

    MoveDetector move_detector;
};

}
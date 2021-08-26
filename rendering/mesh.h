#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "framework/context.h"
#include "rendering/vulkan.h"

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
    MeshRenderer() = default;
    MeshRenderer(const MeshRenderer&) = delete;
    MeshRenderer(MeshRenderer&&) = default;
    MeshRenderer& operator=(const MeshRenderer&) = delete;
    MeshRenderer& operator=(MeshRenderer&&) = default;

    EVENT(VulkanInitialised) {
        assert(!initialised);
        assert(!initialising);
        initialising = true;

        if (!commandPool.has_value()) {
            commandPool = co_await ctx(NewCommandPool{});
        }

        createVertexBuffer(event.physicalDevice, event.device);
        co_await ctx(TransferDataToBuffer{
            .data = std::span(
                reinterpret_cast<const char*>(mesh::detail::vertices.data()),
                mesh::detail::vertices.size() * sizeof(mesh::detail::vertices[0])
            ),
            .dst_buffer = vertexBuffer,
        });

        initialising = false;
        initialised = true;
    }

    EVENT(NewSwapChain) {
        assert(initialised);
        assert(!newSwapChainInProgress);
        newSwapChainInProgress = true;

        if (swapChainInitialised) {
            cleanupSwapChain(event.device);
            swapChainInitialised = false;
        }

        if (!commandBufferHandle.has_value()) {
            commandBufferHandle = co_await ctx(NewCommandBufferHandle{renderOrder});
        }

        createImageViews(event.swapChainImages, event.swapChainImageFormat, event.device);
        createRenderPass(event.swapChainImageFormat, event.device);
        createGraphicsPipeline(event.swapChainExtent, event.device);
        createFramebuffers(event.swapChainExtent, event.device);
        createCommandBuffers(event.swapChainExtent, event.device);

        auto req = UpdateCommandBuffers{
            *commandBufferHandle,
            commandBuffers,
        };
        co_await ctx(req);

        newSwapChainInProgress = false;
        swapChainInitialised = true;
    }

    EVENT(DeleteSwapChain) {
        assert(!newSwapChainInProgress);
        cleanup(event.device);
        co_return;
    }

private:
    void createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device);

    void createImageViews(std::span<VkImage> swapChainImages, VkFormat swapChainImageFormat, VkDevice device);
    void createRenderPass(VkFormat swapChainImageFormat, VkDevice device);
    void createGraphicsPipeline(VkExtent2D swapChainExtent, VkDevice device);
    void createFramebuffers(VkExtent2D swapChainExtent, VkDevice device);
    void createCommandBuffers(VkExtent2D swapChainExtent, VkDevice device);
    void cleanupSwapChain(VkDevice device);
    void cleanup(VkDevice device);

    VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

    double renderOrder = 0.0;

    std::optional<VkCommandPool> commandPool;
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
    bool initialised = false;
    bool initialising = false;
};

}
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

struct Mesh {
    std::vector<Vertex> vertices;
};

struct AddMesh {
    using ResponseT = void;
    Mesh mesh;
};

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
        commandBufferHandle(ctx.request_sync(NewCommandBufferHandle{renderOrder}))
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
        co_return;
    }

    REQUEST(AddMesh) {
        verticesChanged = true;
        vertices.insert(
            vertices.end(),
            request.mesh.vertices.begin(),
            request.mesh.vertices.end()
        );
        co_return;
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
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    CommandBufferHandle commandBufferHandle;

    bool newSwapChainInProgress = false;

    bool verticesChanged = false;
    std::vector<pt::Vertex> vertices;

    MoveDetector move_detector;
};

}
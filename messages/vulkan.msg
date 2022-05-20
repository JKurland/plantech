import VkCommandPool
import VkCommandBuffer
import VkBuffer
import VkImage
import VkFormat
import VkDevice
import VkPhysicalDevice

event NewFrame {}

request NewCommandBufferHandle -> CommandBufferHandle {}
request NewCommandPool -> VkCommandPool {}

// returns whether there were previously buffers registered with this handle
request UpdateCommandBuffers -> bool {
    // handle from NewCommandBufferHandle
    CommandBufferHandle handle

    // one buffer per swapchain image. Must be in the same order as the
    // swapchain images in NewSwapChain. VulkanRendering does not
    // take ownership of these commandBuffers, it is still your responsibility
    // to clean them up.
    list[VkCommandBuffer] commandBuffers
}

request TransferDataToBuffer -> void {
    list[byte] data
    VkBuffer dst_buffer
}

data SwapChainInfo {
    list[VkImage] images
    VkFormat imageFormat
    Extent2D extent
}

data CommandBufferHandle {
    usize idx    
}

event NewSwapChain {
    SwapChainInfo info
    VkDevice device
}

// Emitted before rendering, another render will not
// be kicked off, nor another PreRender emitted,
// before all handlers have finished with PreRender.
event PreRender {}

request GetVulkanPhysicalDevice -> VkPhysicalDevice {}
request GetVulkanDevice -> VkDevice {}
request GetSwapChainInfo -> SwapChainInfo {}
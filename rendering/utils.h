#pragma once

#include <vulkan/vulkan.h>

namespace pt::vkutils {

void createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory
);

void copyBuffer(
    VkDevice device,
    VkCommandPool commandPool,
    VkQueue queue,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size
);

uint32_t findMemoryType(
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties,
    VkPhysicalDevice physicalDevice
);

}
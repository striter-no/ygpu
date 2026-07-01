#pragma once
#include <device/device.hpp>
#include <errors.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace yst::core {

class Buffer {
private:
    VmaAllocation allocation = VK_NULL_HANDLE;
    void* mappedData = nullptr;
    VkDeviceSize size = 0;

    friend std::pair<Buffer, CustomError> CreateBuffer(
        Device& device, VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible);

public:
    VkBuffer buffer = VK_NULL_HANDLE;

    Buffer() = default;

    CustomError UploadData(const void* data, size_t dataSize);

    void Destroy(Device& device);
};

std::pair<Buffer, CustomError> CreateBuffer(
    Device& device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    bool hostVisible = false);

} // namespace yst::core

#include <buffer/buffer.hpp>
#include <cstring>

namespace yst::core {

std::pair<Buffer, CustomError> CreateBuffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible)
{
    Buffer out;
    out.size = size;

    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    if (hostVisible) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VmaAllocationInfo vmaAllocInfo;
    if (vmaCreateBuffer(device.Allocator, &bufferInfo, &allocInfo, &out.buffer, &out.allocation, &vmaAllocInfo) != VK_SUCCESS) {
        return { out, CustomError(50, "Failed to create buffer") };
    }

    if (hostVisible) {
        out.mappedData = vmaAllocInfo.pMappedData;
    }

    return { std::move(out), CustomError() };
}

CustomError Buffer::UploadData(const void* data, size_t dataSize)
{
    if (!mappedData) {
        return CustomError(51, "Buffer is not host-visible! Cannot upload data directly from CPU.");
    }
    if (dataSize > size) {
        return CustomError(52, "Data size exceeds buffer capacity!");
    }

    std::memcpy(mappedData, data, dataSize);
    return CustomError();
}

void Buffer::Destroy(Device& device)
{
    if (buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(device.Allocator, buffer, allocation);
        buffer = VK_NULL_HANDLE;
    }
}

} // namespace yst::core

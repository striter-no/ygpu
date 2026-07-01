#include <buffer/buffer.hpp>
#include <cstring>

namespace yst::core {

std::pair<Buffer, CustomError> CreateBuffer(Device& device, const BufferConfig& config)
{
    Buffer out;
    out.size = config.Size;

    if (config.Size == 0) {
        return { std::move(out),
            CustomError(ErrorCode::BufferCreationFailed,
                "Cannot create a zero-sized buffer") };
    }

    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = config.Size;
    bufferInfo.usage = config.Usage;
    bufferInfo.sharingMode = config.SharingMode;
    if (config.SharingMode == VK_SHARING_MODE_CONCURRENT
        && !config.QueueFamilies.empty()) {
        bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(config.QueueFamilies.size());
        bufferInfo.pQueueFamilyIndices = config.QueueFamilies.data();
    }

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = config.MemoryUsage;
    allocInfo.flags = config.AllocationFlags;
    allocInfo.requiredFlags = config.RequiredMemoryFlags;
    allocInfo.preferredFlags = config.PreferredMemoryFlags;

    VmaAllocationInfo vmaAllocInfo;
    if (vmaCreateBuffer(device.Allocator, &bufferInfo, &allocInfo,
            &out.buffer, &out.allocation, &vmaAllocInfo)
        != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::BufferCreationFailed,
                "Failed to create buffer") };
    }

    // VMA reports pMappedData when either MAPPED flag was requested or
    // VMA decided to keep the allocation persistently mapped. We treat both
    // as "mapped" for the purposes of UploadData.
    if (vmaAllocInfo.pMappedData) {
        out.mappedData = vmaAllocInfo.pMappedData;
    }

    return { std::move(out), CustomError() };
}

CustomError Buffer::UploadData(const void* data, size_t dataSize)
{
    if (!mappedData) {
        return CustomError(ErrorCode::BufferNotHostVisible,
            "Buffer is not host-visible! Cannot upload data directly from CPU.");
    }
    if (dataSize > size) {
        return CustomError(ErrorCode::BufferSizeExceeded,
            "Data size exceeds buffer capacity!");
    }

    std::memcpy(mappedData, data, dataSize);
    return CustomError();
}

void Buffer::Destroy(Device& device)
{
    if (buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(device.Allocator, buffer, allocation);
        buffer = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
        mappedData = nullptr;
        size = 0;
    }
}

} // namespace yst::core

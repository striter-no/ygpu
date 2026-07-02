#include <buffer/buffer.hpp>
#include <cstring>

namespace yst::core {

Buffer::Buffer(Buffer&& other) noexcept
{
    device_ = other.device_;
    buffer = other.buffer;
    allocation_ = other.allocation_;
    mappedData_ = other.mappedData_;
    size_ = other.size_;

    other.device_ = nullptr;
    other.buffer = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.mappedData_ = nullptr;
    other.size_ = 0;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        buffer = other.buffer;
        allocation_ = other.allocation_;
        mappedData_ = other.mappedData_;
        size_ = other.size_;

        other.device_ = nullptr;
        other.buffer = VK_NULL_HANDLE;
        other.allocation_ = VK_NULL_HANDLE;
        other.mappedData_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

void Buffer::Destroy()
{
    if (buffer != VK_NULL_HANDLE && device_) {
        vmaDestroyBuffer(device_->Allocator, buffer, allocation_);
        buffer = VK_NULL_HANDLE;
        allocation_ = VK_NULL_HANDLE;
        mappedData_ = nullptr;
        size_ = 0;
        device_ = nullptr;
    }
}

std::pair<Buffer, CustomError> CreateBuffer(Device& device, const BufferConfig& config)
{
    Buffer out;
    out.device_ = &device;
    out.size_ = config.Size;

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
            &out.buffer, &out.allocation_, &vmaAllocInfo)
        != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::BufferCreationFailed,
                "Failed to create buffer") };
    }

    if (vmaAllocInfo.pMappedData) {
        out.mappedData_ = vmaAllocInfo.pMappedData;
    }

    return { std::move(out), CustomError() };
}

CustomError Buffer::UploadData(const void* data, size_t dataSize)
{
    if (!mappedData_) {
        return CustomError(ErrorCode::BufferNotHostVisible,
            "Buffer is not host-visible! Cannot upload data directly from CPU.");
    }
    if (dataSize > size_) {
        return CustomError(ErrorCode::BufferSizeExceeded,
            "Data size exceeds buffer capacity!");
    }

    std::memcpy(mappedData_, data, dataSize);
    return CustomError();
}

// ---- Convenience overloads -----------------------------------------------

std::pair<Buffer, CustomError> CreateUniformBuffer(Device& device, VkDeviceSize size)
{
    BufferConfig cfg;
    cfg.Size = size;
    cfg.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    cfg.HostVisible();
    return CreateBuffer(device, cfg);
}

std::pair<Buffer, CustomError> CreateVertexBuffer(Device& device, VkDeviceSize size,
    bool hostVisible)
{
    BufferConfig cfg;
    cfg.Size = size;
    cfg.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (hostVisible)
        cfg.HostVisible();
    return CreateBuffer(device, cfg);
}

std::pair<Buffer, CustomError> CreateIndexBuffer(Device& device, VkDeviceSize size,
    bool hostVisible)
{
    BufferConfig cfg;
    cfg.Size = size;
    cfg.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (hostVisible)
        cfg.HostVisible();
    return CreateBuffer(device, cfg);
}

std::pair<Buffer, CustomError> CreateStagingBuffer(Device& device, VkDeviceSize size)
{
    BufferConfig cfg;
    cfg.Size = size;
    cfg.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    cfg.HostVisible();
    return CreateBuffer(device, cfg);
}

} // namespace yst::core

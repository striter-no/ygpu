#pragma once
#include <device/device.hpp>
#include <errors.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace yst::core {

/// Layer 2 configuration for a VMA-allocated buffer.
struct BufferConfig {
    VkDeviceSize Size = 0;
    VkBufferUsageFlags Usage = 0;

    VkSharingMode SharingMode = VK_SHARING_MODE_EXCLUSIVE;
    std::vector<uint32_t> QueueFamilies;

    VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationCreateFlags AllocationFlags = 0;
    VkMemoryPropertyFlags RequiredMemoryFlags = 0;
    VkMemoryPropertyFlags PreferredMemoryFlags = 0;

    /// Convenience: configure as a persistently-mapped, host-writable
    /// staging buffer.
    BufferConfig& HostVisible() noexcept
    {
        AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        return *this;
    }
};

/// Owning wrapper around a Vma-allocated buffer. RAII: destruction
/// automatically frees the underlying VkBuffer + VmaAllocation.
class Buffer {
public:
    VkBuffer buffer = VK_NULL_HANDLE;

    Buffer() = default;
    ~Buffer() { Destroy(); }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    /// Free the buffer. Safe to call multiple times; safe on a moved-from
    /// instance. Uses the device stored at creation time.
    void Destroy();

    CustomError UploadData(const void* data, size_t dataSize);

    bool IsMapped() const noexcept { return mappedData_ != nullptr; }
    void* MappedData() noexcept { return mappedData_; }
    const void* MappedData() const noexcept { return mappedData_; }
    VkDeviceSize Size() const noexcept { return size_; }

private:
    Device* device_ = nullptr;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    void* mappedData_ = nullptr;
    VkDeviceSize size_ = 0;

    friend std::pair<Buffer, CustomError> CreateBuffer(
        Device& device, const BufferConfig& config);
};

std::pair<Buffer, CustomError> CreateBuffer(
    Device& device, const BufferConfig& config);

// ---- Convenience overloads (Layer 1) ------------------------------------
// Each helper constructs a BufferConfig with sensible defaults for the
// given use case. All return host-visible, persistently-mapped buffers
// (i.e. CPU-writable via UploadData) unless noted otherwise.

/// Uniform buffer (UBO): host-visible, persistently mapped.
std::pair<Buffer, CustomError> CreateUniformBuffer(Device& device, VkDeviceSize size);

/// Vertex buffer: host-visible by default for simple examples. Pass
/// hostVisible=false to allocate GPU-local memory (requires staging).
std::pair<Buffer, CustomError> CreateVertexBuffer(Device& device, VkDeviceSize size,
    bool hostVisible = true);

/// Index buffer: same defaults as vertex buffer.
std::pair<Buffer, CustomError> CreateIndexBuffer(Device& device, VkDeviceSize size,
    bool hostVisible = true);

/// Staging buffer: transfer-src + host-visible, used as the source of
/// GPU-side copies (e.g. uploading to a GPU-local vertex buffer or image).
std::pair<Buffer, CustomError> CreateStagingBuffer(Device& device, VkDeviceSize size);

/// Backwards-compatible overload: builds a BufferConfig from the most
/// common subset of parameters.
inline std::pair<Buffer, CustomError> CreateBuffer(
    Device& device, VkDeviceSize size, VkBufferUsageFlags usage,
    bool hostVisible = false)
{
    BufferConfig cfg;
    cfg.Size = size;
    cfg.Usage = usage;
    if (hostVisible)
        cfg.HostVisible();
    return CreateBuffer(device, cfg);
}

} // namespace yst::core

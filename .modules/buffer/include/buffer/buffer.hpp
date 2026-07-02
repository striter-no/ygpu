// yst buffer module
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <device/device.hpp>
#include <errors.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace yst::core {

class Buffer;

/// Named presets for fast, common BufferConfig defaults.
enum class BufferPreset {
    Empty = 0,
    Uniform,
    Vertex,
    Index,
    Staging,
};

/// Plain data configuration for a buffer. Callers may use CreateConfig(preset)
/// for defaults and then modify fields directly, or use BufferBuilder when a
/// chainable API is clearer.
struct BufferConfig {
    VkDeviceSize Size = 0;
    VkBufferUsageFlags Usage = 0;
    VkSharingMode SharingMode = VK_SHARING_MODE_EXCLUSIVE;
    std::vector<uint32_t> QueueFamilies;
    VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationCreateFlags AllocationFlags = 0;
    VkMemoryPropertyFlags RequiredMemoryFlags = 0;
    VkMemoryPropertyFlags PreferredMemoryFlags = 0;
    bool HostVisible = false;
};

BufferConfig CreateConfig(BufferPreset preset);

std::pair<Buffer, CustomError> CreateBuffer(
    Device& device, const BufferConfig& config);

/// Fluent builder for BufferConfig. Build() returns a config only; resource
/// creation stays in CreateBuffer(device, config).
class BufferBuilder {
public:
    BufferBuilder() = default;
    explicit BufferBuilder(BufferPreset preset)
        : cfg_(CreateConfig(preset))
    {
    }
    explicit BufferBuilder(BufferConfig config)
        : cfg_(std::move(config))
    {
    }

    BufferBuilder& WithSize(VkDeviceSize size)
    {
        cfg_.Size = size;
        return *this;
    }
    BufferBuilder& WithUsage(VkBufferUsageFlags usage)
    {
        cfg_.Usage = usage;
        return *this;
    }
    BufferBuilder& AddUsageFlags(VkBufferUsageFlags usage)
    {
        cfg_.Usage |= usage;
        return *this;
    }
    BufferBuilder& WithSharingMode(VkSharingMode mode)
    {
        cfg_.SharingMode = mode;
        return *this;
    }
    BufferBuilder& WithQueueFamilies(std::vector<uint32_t> families)
    {
        cfg_.QueueFamilies = std::move(families);
        return *this;
    }
    BufferBuilder& WithMemoryUsage(VmaMemoryUsage usage)
    {
        cfg_.MemoryUsage = usage;
        return *this;
    }
    BufferBuilder& WithAllocationFlags(VmaAllocationCreateFlags flags)
    {
        cfg_.AllocationFlags = flags;
        cfg_.HostVisible = (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;
        return *this;
    }
    BufferBuilder& WithRequiredMemoryFlags(VkMemoryPropertyFlags flags)
    {
        cfg_.RequiredMemoryFlags = flags;
        return *this;
    }
    BufferBuilder& WithPreferredMemoryFlags(VkMemoryPropertyFlags flags)
    {
        cfg_.PreferredMemoryFlags = flags;
        return *this;
    }

    /// Configure as a persistently-mapped, host-writable buffer.
    BufferBuilder& HostVisible()
    {
        cfg_.AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        cfg_.HostVisible = true;
        return *this;
    }

    BufferConfig Build() const { return cfg_; }

private:
    BufferConfig cfg_;
};

class Buffer {
public:
    VkBuffer buffer = VK_NULL_HANDLE;

    Buffer() = default;
    ~Buffer() { Destroy(); }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

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

inline std::pair<Buffer, CustomError> CreateUniformBuffer(Device& device, VkDeviceSize size)
{
    auto cfg = CreateConfig(BufferPreset::Uniform);
    cfg.Size = size;
    return CreateBuffer(device, cfg);
}

inline std::pair<Buffer, CustomError> CreateVertexBuffer(Device& device, VkDeviceSize size,
    bool hostVisible = true)
{
    auto cfg = CreateConfig(BufferPreset::Vertex);
    cfg.Size = size;
    if (hostVisible) {
        cfg.AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        cfg.HostVisible = true;
    }
    return CreateBuffer(device, cfg);
}

inline std::pair<Buffer, CustomError> CreateIndexBuffer(Device& device, VkDeviceSize size,
    bool hostVisible = true)
{
    auto cfg = CreateConfig(BufferPreset::Index);
    cfg.Size = size;
    if (hostVisible) {
        cfg.AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        cfg.HostVisible = true;
    }
    return CreateBuffer(device, cfg);
}

inline std::pair<Buffer, CustomError> CreateStagingBuffer(Device& device, VkDeviceSize size)
{
    auto cfg = CreateConfig(BufferPreset::Staging);
    cfg.Size = size;
    return CreateBuffer(device, cfg);
}

} // namespace yst::core

#pragma once
#include <device/device.hpp>
#include <errors.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace yst::core {

/// Layer 2 configuration for a VMA-allocated buffer.
///
/// Defaults reproduce the historical "EXCLUSIVE sharing, AUTO memory usage,
/// persistent mapping when hostVisible==true" code path. The hostVisible
/// boolean is preserved as a convenience preset via the
/// `HostAccessSequentialWrite + Mapped` flag combination.
struct BufferConfig {
    VkDeviceSize Size = 0;
    VkBufferUsageFlags Usage = 0;

    /// Sharing mode between queue families. EXCLUSIVE is correct for the
    /// single-graphics-queue engines that yst currently targets.
    VkSharingMode SharingMode = VK_SHARING_MODE_EXCLUSIVE;
    std::vector<uint32_t> QueueFamilies; ///< Ignored unless SharingMode == CONCURRENT.

    /// VMA memory usage hint. AUTO lets VMA pick the optimal heap.
    VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO;

    /// VMA allocation flags. For a host-visible staging buffer, set
    /// (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
    ///  VMA_ALLOCATION_CREATE_MAPPED_BIT) — or use the HostVisible() helper.
    VmaAllocationCreateFlags AllocationFlags = 0;

    /// Hard memory property requirements (e.g. HOST_VISIBLE_BIT).
    VkMemoryPropertyFlags RequiredMemoryFlags = 0;

    /// Soft memory property preferences (e.g. HOST_COHERENT_BIT).
    VkMemoryPropertyFlags PreferredMemoryFlags = 0;

    /// Convenience: configure as a persistently-mapped, host-writable
    /// staging buffer. Equivalent to setting AllocationFlags to
    /// (HOST_ACCESS_SEQUENTIAL_WRITE | MAPPED).
    BufferConfig& HostVisible() noexcept
    {
        AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        return *this;
    }
};

class Buffer {
private:
    VmaAllocation allocation = VK_NULL_HANDLE;
    void* mappedData = nullptr;
    VkDeviceSize size = 0;

    friend std::pair<Buffer, CustomError> CreateBuffer(
        Device& device, const BufferConfig& config);

public:
    VkBuffer buffer = VK_NULL_HANDLE;

    Buffer() = default;

    /// Upload `dataSize` bytes from `data` into the mapped region. Returns
    /// BufferNotHostVisible if the buffer wasn't created with HostVisible(),
    /// or BufferSizeExceeded if `dataSize` exceeds the allocated size.
    CustomError UploadData(const void* data, size_t dataSize);

    /// True if the buffer is currently persistently mapped on the host.
    bool IsMapped() const noexcept { return mappedData != nullptr; }

    /// Pointer to the mapped region, or nullptr if not mapped.
    void* MappedData() noexcept { return mappedData; }
    const void* MappedData() const noexcept { return mappedData; }

    VkDeviceSize Size() const noexcept { return size; }

    void Destroy(Device& device);
};

/// Full-config entry point (Layer 2). Always prefer this for new code.
std::pair<Buffer, CustomError> CreateBuffer(
    Device& device, const BufferConfig& config);

/// Convenience overload (Layer 1): builds a BufferConfig from the most
/// common subset of parameters. `hostVisible=true` selects persistently-
/// mapped, sequential-write host memory — same as the historical behaviour.
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

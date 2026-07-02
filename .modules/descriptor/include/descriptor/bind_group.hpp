#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <descriptor/bind_group_layout.hpp>
#include <descriptor/descriptor_pool.hpp>
#include <device/device.hpp>
#include <errors.hpp>

namespace yst::core {

/// One resource binding in a BindGroup. Mirrors wgpu::BindGroupEntry.
/// Depending on the binding type declared in the layout, different
/// fields are used:
///
///   UniformBuffer / StorageBuffer / ReadonlyStorageBuffer:
///       Buffer, Offset, Range
///   SampledTexture / StorageTexture:
///       ImageView, ImageLayout
///   Sampler:
///       Sampler
///   CombinedTextureSampler:
///       ImageView, ImageLayout, Sampler
struct BindGroupEntry {
    uint32_t Binding = 0;

    // Buffer bindings
    VkBuffer Buffer = VK_NULL_HANDLE;
    VkDeviceSize Offset = 0;
    VkDeviceSize Range = VK_WHOLE_SIZE;

    // Image bindings
    VkImageView ImageView = VK_NULL_HANDLE;
    VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Sampler bindings
    VkSampler Sampler = VK_NULL_HANDLE;
};

/// Configuration for a BindGroup: a layout + the actual resources bound
/// to each slot.
struct BindGroupConfig {
    BindGroupLayout* Layout = nullptr;
    std::vector<BindGroupEntry> Entries;
};

/// Non-owning wrapper around a VkDescriptorSet.
///
/// A BindGroup does NOT own its underlying VkDescriptorSet — the set
/// remains owned by the DescriptorPool it was allocated from. Destroying
/// or resetting the pool invalidates all BindGroups allocated from it.
/// Call Free() to return the set to the pool before the pool is reset
/// (only meaningful if the pool was created with FREE_DESCRIPTOR_SET_BIT).
class BindGroup {
public:
    VkDescriptorSet set = VK_NULL_HANDLE;
    BindGroupLayout* layout = nullptr;

    BindGroup() = default;

    /// Free the set back to its pool. No-op if the set is already null.
    /// Only works if the pool was created with FREE_DESCRIPTOR_SET_BIT.
    CustomError Free(Device& device, DescriptorPool& pool);

    /// Re-write the bound resources. Useful for updating a per-frame
    /// BindGroup without allocating a new set.
    CustomError Update(Device& device, const BindGroupConfig& config);
};

/// Allocate a single BindGroup from a pool, then immediately write the
/// configured resources into it.
std::pair<BindGroup, CustomError> CreateBindGroup(
    Device& device, DescriptorPool& pool, const BindGroupConfig& config);

/// Allocate a BindGroup from a pool without writing any resources. Caller
/// is expected to call BindGroup::Update() afterwards. Useful when the
/// resources aren't known at allocation time.
std::pair<BindGroup, CustomError> AllocateBindGroup(
    Device& device, DescriptorPool& pool, BindGroupLayout& layout);

} // namespace yst::core

#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace yst::core {

/// WebGPU-style binding type. Maps 1:1 to a Vulkan descriptor type.
enum class BindingType {
    UniformBuffer, // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    StorageBuffer, // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER (read-write)
    ReadonlyStorageBuffer, // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER (read-only)
    SampledTexture, // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    StorageTexture, // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    Sampler, // VK_DESCRIPTOR_TYPE_SAMPLER
    CombinedTextureSampler, // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
};

/// Shader stage visibility flags. Used as bitmask in BindingLayoutEntry.
namespace ShaderStageBits {
    constexpr VkShaderStageFlags None = 0;
    constexpr VkShaderStageFlags Vertex = VK_SHADER_STAGE_VERTEX_BIT;
    constexpr VkShaderStageFlags Fragment = VK_SHADER_STAGE_FRAGMENT_BIT;
    constexpr VkShaderStageFlags Compute = VK_SHADER_STAGE_COMPUTE_BIT;
    constexpr VkShaderStageFlags AllGraphics = VK_SHADER_STAGE_ALL_GRAPHICS;
    constexpr VkShaderStageFlags All = VK_SHADER_STAGE_ALL;
} // namespace ShaderStageBits

/// Convert a BindingType to the underlying VkDescriptorType.
VkDescriptorType ToVkDescriptorType(BindingType type) noexcept;

/// One entry in a BindGroupLayout. Mirrors wgpu::BindGroupLayoutEntry.
struct BindingLayoutEntry {
    uint32_t Binding = 0;
    BindingType Type = BindingType::UniformBuffer;
    VkShaderStageFlags Visibility = ShaderStageBits::AllGraphics;
    uint32_t Count = 1; ///< >1 for array bindings.

    /// Convenience: configure as a UBO visible in all graphics stages.
    BindingLayoutEntry& AsUniformBuffer(uint32_t binding,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics) noexcept
    {
        Binding = binding;
        Type = BindingType::UniformBuffer;
        Visibility = visibility;
        Count = 1;
        return *this;
    }

    BindingLayoutEntry& AsStorageBuffer(uint32_t binding, bool readOnly = false,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics) noexcept
    {
        Binding = binding;
        Type = readOnly ? BindingType::ReadonlyStorageBuffer
                        : BindingType::StorageBuffer;
        Visibility = visibility;
        Count = 1;
        return *this;
    }

    BindingLayoutEntry& AsSampler(uint32_t binding,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics) noexcept
    {
        Binding = binding;
        Type = BindingType::Sampler;
        Visibility = visibility;
        Count = 1;
        return *this;
    }

    BindingLayoutEntry& AsSampledTexture(uint32_t binding,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics) noexcept
    {
        Binding = binding;
        Type = BindingType::SampledTexture;
        Visibility = visibility;
        Count = 1;
        return *this;
    }

    BindingLayoutEntry& AsCombinedTextureSampler(uint32_t binding,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics) noexcept
    {
        Binding = binding;
        Type = BindingType::CombinedTextureSampler;
        Visibility = visibility;
        Count = 1;
        return *this;
    }
};

} // namespace yst::core

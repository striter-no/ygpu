#pragma once
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace yst::core {

/// Layer 2 configuration for a Vma-allocated VkImage.
struct ImageConfig {
    VkImageType Type = VK_IMAGE_TYPE_2D;
    VkFormat Format = VK_FORMAT_R8G8B8A8_SRGB;
    VkExtent3D Extent { 1, 1, 1 };
    uint32_t MipLevels = 1;
    uint32_t ArrayLayers = 1;
    VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags Usage = VK_IMAGE_USAGE_SAMPLED_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkSharingMode SharingMode = VK_SHARING_MODE_EXCLUSIVE;
    std::vector<uint32_t> QueueFamilies; ///< Ignored unless SharingMode == CONCURRENT.
    VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationCreateFlags AllocationFlags = 0;

    /// Convenience: configure as a 2D sampled texture with N mip levels.
    ImageConfig& As2DTexture(uint32_t width, uint32_t height,
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
        uint32_t mipLevels = 1) noexcept
    {
        Type = VK_IMAGE_TYPE_2D;
        Format = format;
        Extent = { width, height, 1 };
        MipLevels = mipLevels;
        ArrayLayers = 1;
        Samples = VK_SAMPLE_COUNT_1_BIT;
        Tiling = VK_IMAGE_TILING_OPTIMAL;
        Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (mipLevels > 1)
            Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        return *this;
    }

    /// Convenience: configure as a depth/stencil image.
    ImageConfig& AsDepthAttachment(uint32_t width, uint32_t height,
        VkFormat format = VK_FORMAT_D32_SFLOAT,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT) noexcept
    {
        Type = VK_IMAGE_TYPE_2D;
        Format = format;
        Extent = { width, height, 1 };
        MipLevels = 1;
        ArrayLayers = 1;
        Samples = samples;
        Tiling = VK_IMAGE_TILING_OPTIMAL;
        Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        return *this;
    }
};

/// Layer 2 configuration for a VkImageView.
struct ImageViewConfig {
    VkImageViewType Type = VK_IMAGE_VIEW_TYPE_2D;
    VkFormat Format = VK_FORMAT_UNDEFINED; ///< VK_FORMAT_UNDEFINED = inherit from image.
    VkComponentMapping Components {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY
    };
    VkImageAspectFlags AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t BaseMipLevel = 0;
    uint32_t LevelCount = VK_REMAINING_MIP_LEVELS;
    uint32_t BaseArrayLayer = 0;
    uint32_t LayerCount = VK_REMAINING_ARRAY_LAYERS;
};

/// Layer 2 configuration for a VkSampler.
struct SamplerConfig {
    VkFilter MagFilter = VK_FILTER_LINEAR;
    VkFilter MinFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode MipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode AddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode AddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode AddressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float MipLodBias = 0.0f;
    VkBool32 AnisotropyEnable = VK_FALSE;
    float MaxAnisotropy = 0.0f;
    VkBool32 CompareEnable = VK_FALSE;
    VkCompareOp CompareOp = VK_COMPARE_OP_NEVER;
    float MinLod = 0.0f;
    float MaxLod = VK_LOD_CLAMP_NONE;
    VkBorderColor BorderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    VkBool32 UnnormalizedCoordinates = VK_FALSE;
};

} // namespace yst::core

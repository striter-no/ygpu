// yst image module config
#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace yst::core {

class Image;
class ImageView;
class Sampler;

enum class ImagePreset {
    Texture2D = 0,
    DepthAttachment,
};

enum class ImageViewPreset {
    Color2D = 0,
    Depth2D,
};

enum class SamplerPreset {
    LinearRepeat = 0,
    NearestClamp,
    ShadowCompare,
};

// =====================================================================
// ImageConfig
// =====================================================================

struct ImageConfig {
    VkImageType Type = VK_IMAGE_TYPE_2D;
    VkFormat Format = VK_FORMAT_R8G8B8A8_SRGB;
    VkExtent3D Extent { 1, 1, 1 };
    uint32_t MipLevels = 1;
    uint32_t ArrayLayers = 1;
    VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkSharingMode SharingMode = VK_SHARING_MODE_EXCLUSIVE;
    std::vector<uint32_t> QueueFamilies;
    VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationCreateFlags AllocationFlags = 0;
};

ImageConfig CreateConfig(ImagePreset preset);

class ImageBuilder {
public:
    ImageBuilder() = default;
    explicit ImageBuilder(ImagePreset preset)
        : cfg_(CreateConfig(preset))
    {
    }
    explicit ImageBuilder(ImageConfig config)
        : cfg_(std::move(config))
    {
    }

    ImageBuilder& WithType(VkImageType t)
    {
        cfg_.Type = t;
        return *this;
    }
    ImageBuilder& WithFormat(VkFormat f)
    {
        cfg_.Format = f;
        return *this;
    }
    ImageBuilder& WithExtent(VkExtent3D e)
    {
        cfg_.Extent = e;
        return *this;
    }
    ImageBuilder& WithExtent(uint32_t w, uint32_t h, uint32_t d = 1)
    {
        cfg_.Extent = { w, h, d };
        return *this;
    }
    ImageBuilder& WithMipLevels(uint32_t n)
    {
        cfg_.MipLevels = n;
        return *this;
    }
    ImageBuilder& WithArrayLayers(uint32_t n)
    {
        cfg_.ArrayLayers = n;
        return *this;
    }
    ImageBuilder& WithSamples(VkSampleCountFlagBits s)
    {
        cfg_.Samples = s;
        return *this;
    }
    ImageBuilder& WithTiling(VkImageTiling t)
    {
        cfg_.Tiling = t;
        return *this;
    }
    ImageBuilder& WithUsage(VkImageUsageFlags u)
    {
        cfg_.Usage = u;
        return *this;
    }
    ImageBuilder& AddUsageFlags(VkImageUsageFlags u)
    {
        cfg_.Usage |= u;
        return *this;
    }
    ImageBuilder& WithSharingMode(VkSharingMode m)
    {
        cfg_.SharingMode = m;
        return *this;
    }
    ImageBuilder& WithQueueFamilies(std::vector<uint32_t> f)
    {
        cfg_.QueueFamilies = std::move(f);
        return *this;
    }
    ImageBuilder& WithInitialLayout(VkImageLayout l)
    {
        cfg_.InitialLayout = l;
        return *this;
    }
    ImageBuilder& WithMemoryUsage(VmaMemoryUsage u)
    {
        cfg_.MemoryUsage = u;
        return *this;
    }
    ImageBuilder& WithAllocationFlags(VmaAllocationCreateFlags f)
    {
        cfg_.AllocationFlags = f;
        return *this;
    }

    /// Configure as a 2D sampled texture with N mip levels.
    ImageBuilder& As2DTexture(uint32_t width, uint32_t height,
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
        uint32_t mipLevels = 1)
    {
        cfg_ = CreateConfig(ImagePreset::Texture2D);
        cfg_.Format = format;
        cfg_.Extent = { width, height, 1 };
        cfg_.MipLevels = mipLevels;
        if (mipLevels > 1)
            cfg_.Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        return *this;
    }

    /// Configure as a depth/stencil image.
    ImageBuilder& AsDepthAttachment(uint32_t width, uint32_t height,
        VkFormat format = VK_FORMAT_D32_SFLOAT,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
    {
        cfg_ = CreateConfig(ImagePreset::DepthAttachment);
        cfg_.Format = format;
        cfg_.Extent = { width, height, 1 };
        cfg_.Samples = samples;
        return *this;
    }

    ImageConfig Build() const { return cfg_; }

private:
    ImageConfig cfg_;
};

// =====================================================================
// ImageViewConfig
// =====================================================================

struct ImageViewConfig {
    VkImageViewType Type = VK_IMAGE_VIEW_TYPE_2D;
    VkFormat Format = VK_FORMAT_UNDEFINED;
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

ImageViewConfig CreateConfig(ImageViewPreset preset);

class ImageViewBuilder {
public:
    ImageViewBuilder() = default;
    explicit ImageViewBuilder(ImageViewPreset preset)
        : cfg_(CreateConfig(preset))
    {
    }
    explicit ImageViewBuilder(ImageViewConfig config)
        : cfg_(config)
    {
    }

    ImageViewBuilder& WithType(VkImageViewType t)
    {
        cfg_.Type = t;
        return *this;
    }
    ImageViewBuilder& WithFormat(VkFormat f)
    {
        cfg_.Format = f;
        return *this;
    }
    ImageViewBuilder& WithComponents(VkComponentMapping c)
    {
        cfg_.Components = c;
        return *this;
    }
    ImageViewBuilder& WithAspectMask(VkImageAspectFlags a)
    {
        cfg_.AspectMask = a;
        return *this;
    }
    ImageViewBuilder& WithBaseMipLevel(uint32_t l)
    {
        cfg_.BaseMipLevel = l;
        return *this;
    }
    ImageViewBuilder& WithLevelCount(uint32_t n)
    {
        cfg_.LevelCount = n;
        return *this;
    }
    ImageViewBuilder& WithBaseArrayLayer(uint32_t l)
    {
        cfg_.BaseArrayLayer = l;
        return *this;
    }
    ImageViewBuilder& WithLayerCount(uint32_t n)
    {
        cfg_.LayerCount = n;
        return *this;
    }

    ImageViewConfig Build() const { return cfg_; }

private:
    ImageViewConfig cfg_;
};

// =====================================================================
// SamplerConfig
// =====================================================================

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

SamplerConfig CreateConfig(SamplerPreset preset);

class SamplerBuilder {
public:
    SamplerBuilder() = default;
    explicit SamplerBuilder(SamplerPreset preset)
        : cfg_(CreateConfig(preset))
    {
    }
    explicit SamplerBuilder(SamplerConfig config)
        : cfg_(config)
    {
    }

    SamplerBuilder& WithMagFilter(VkFilter f)
    {
        cfg_.MagFilter = f;
        return *this;
    }
    SamplerBuilder& WithMinFilter(VkFilter f)
    {
        cfg_.MinFilter = f;
        return *this;
    }
    SamplerBuilder& WithFilters(VkFilter mag, VkFilter min)
    {
        cfg_.MagFilter = mag;
        cfg_.MinFilter = min;
        return *this;
    }
    SamplerBuilder& WithMipmapMode(VkSamplerMipmapMode m)
    {
        cfg_.MipmapMode = m;
        return *this;
    }
    SamplerBuilder& WithAddressModeU(VkSamplerAddressMode m)
    {
        cfg_.AddressModeU = m;
        return *this;
    }
    SamplerBuilder& WithAddressModeV(VkSamplerAddressMode m)
    {
        cfg_.AddressModeV = m;
        return *this;
    }
    SamplerBuilder& WithAddressModeW(VkSamplerAddressMode m)
    {
        cfg_.AddressModeW = m;
        return *this;
    }
    SamplerBuilder& WithAddressModes(VkSamplerAddressMode u, VkSamplerAddressMode v,
        VkSamplerAddressMode w = VK_SAMPLER_ADDRESS_MODE_REPEAT)
    {
        cfg_.AddressModeU = u;
        cfg_.AddressModeV = v;
        cfg_.AddressModeW = w;
        return *this;
    }
    SamplerBuilder& WithMipLodBias(float b)
    {
        cfg_.MipLodBias = b;
        return *this;
    }
    SamplerBuilder& WithAnisotropy(bool enabled, float maxAniso = 16.0f)
    {
        cfg_.AnisotropyEnable = enabled ? VK_TRUE : VK_FALSE;
        cfg_.MaxAnisotropy = maxAniso;
        return *this;
    }
    SamplerBuilder& WithCompare(bool enabled, VkCompareOp op = VK_COMPARE_OP_LESS)
    {
        cfg_.CompareEnable = enabled ? VK_TRUE : VK_FALSE;
        cfg_.CompareOp = op;
        return *this;
    }
    SamplerBuilder& WithLodRange(float minLod, float maxLod)
    {
        cfg_.MinLod = minLod;
        cfg_.MaxLod = maxLod;
        return *this;
    }
    SamplerBuilder& WithBorderColor(VkBorderColor c)
    {
        cfg_.BorderColor = c;
        return *this;
    }
    SamplerBuilder& WithUnnormalizedCoordinates(bool enabled)
    {
        cfg_.UnnormalizedCoordinates = enabled ? VK_TRUE : VK_FALSE;
        return *this;
    }

    SamplerConfig Build() const { return cfg_; }

private:
    SamplerConfig cfg_;
};

} // namespace yst::core

// yst texture module
#pragma once

#include <cstdint>
#include <utility>

#include <device/device.hpp>
#include <errors.hpp>
#include <image/image.hpp>
#include <image/sampler.hpp>
#include <vulkan/vulkan.h>

namespace yst::core {

class Texture2D;

enum class Texture2DPreset {
    Rgba8Unorm = 0,
    Rgba8Srgb,
    MipmappedRgba8Srgb,
};

struct Texture2DConfig {
    const void* Pixels = nullptr;
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t Channels = 4;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    bool GenerateMipmaps = false;
    SamplerConfig SamplerCfg {};
};

Texture2DConfig CreateConfig(Texture2DPreset preset);

std::pair<Texture2D, CustomError> CreateTexture2D(
    Device& device, const Texture2DConfig& config);

class Texture2DBuilder {
public:
    Texture2DBuilder() = default;
    explicit Texture2DBuilder(Texture2DPreset preset)
        : cfg_(CreateConfig(preset))
    {
    }
    explicit Texture2DBuilder(Texture2DConfig config)
        : cfg_(config)
    {
    }

    Texture2DBuilder& WithPixels(const void* pixels)
    {
        cfg_.Pixels = pixels;
        return *this;
    }
    Texture2DBuilder& WithExtent(uint32_t width, uint32_t height)
    {
        cfg_.Width = width;
        cfg_.Height = height;
        return *this;
    }
    Texture2DBuilder& WithChannels(uint32_t channels)
    {
        cfg_.Channels = channels;
        return *this;
    }
    Texture2DBuilder& WithFormat(VkFormat format)
    {
        cfg_.Format = format;
        return *this;
    }
    Texture2DBuilder& WithMipmaps(bool enabled = true)
    {
        cfg_.GenerateMipmaps = enabled;
        return *this;
    }
    Texture2DBuilder& WithSamplerConfig(SamplerConfig samplerConfig)
    {
        cfg_.SamplerCfg = samplerConfig;
        return *this;
    }

    Texture2DConfig Build() const { return cfg_; }

private:
    Texture2DConfig cfg_;
};

/// Owned combination of Image + ImageView + Sampler. RAII — destruction
/// automatically frees all three members.
class Texture2D {
public:
    Image image;
    ImageView view;
    Sampler sampler;

    Texture2D() = default;
    ~Texture2D() = default;

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&&) noexcept = default;
    Texture2D& operator=(Texture2D&&) noexcept = default;

    /// Explicit early destruction. Optional — the destructor handles it.
    void Destroy()
    {
        sampler.Destroy();
        view.Destroy();
        image.Destroy();
    }
};

} // namespace yst::core

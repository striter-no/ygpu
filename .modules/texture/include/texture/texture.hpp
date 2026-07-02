// yst texture module header — RAII version (v2)
// Members (Image, ImageView, Sampler) are themselves RAII, so Texture2D
// auto-cleans on scope exit. The explicit Destroy() method is optional.
#pragma once
#include <image/image.hpp>
#include <image/sampler.hpp>

#include <device/device.hpp>
#include <errors.hpp>
#include <utility>
#include <vulkan/vulkan.h>

namespace yst::core {

/// Owned combination of Image + ImageView + Sampler. RAII — destruction
/// automatically frees all three members.
class Texture2D {
public:
    Image image;
    ImageView view;
    Sampler sampler;

    Texture2D() = default;
    ~Texture2D() = default; // members auto-destruct via their own RAII

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&&) noexcept = default;
    Texture2D& operator=(Texture2D&&) noexcept = default;

    /// Explicit early destruction. Optional — the destructor handles it.
    void Destroy()
    {
        view.Destroy();
        image.Destroy();
        sampler.Destroy();
    }
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

std::pair<Texture2D, CustomError> CreateTexture2D(
    Device& device, const Texture2DConfig& config);

} // namespace yst::core

#include <image/sampler.hpp>

namespace yst::core {

Sampler::Sampler(Sampler&& other) noexcept
{
    device_ = other.device_;
    sampler = other.sampler;
    other.device_ = nullptr;
    other.sampler = VK_NULL_HANDLE;
}

Sampler& Sampler::operator=(Sampler&& other) noexcept
{
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        sampler = other.sampler;
        other.device_ = nullptr;
        other.sampler = VK_NULL_HANDLE;
    }
    return *this;
}

void Sampler::Destroy()
{
    if (sampler != VK_NULL_HANDLE && device_) {
        vkDestroySampler(device_->LogicalDevice, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
        device_ = nullptr;
    }
}

std::pair<Sampler, CustomError> CreateSampler(
    Device& device, const SamplerConfig& config)
{
    Sampler out;
    out.device_ = &device;

    VkSamplerCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = config.MagFilter;
    info.minFilter = config.MinFilter;
    info.mipmapMode = config.MipmapMode;
    info.addressModeU = config.AddressModeU;
    info.addressModeV = config.AddressModeV;
    info.addressModeW = config.AddressModeW;
    info.mipLodBias = config.MipLodBias;
    info.anisotropyEnable = config.AnisotropyEnable;
    info.maxAnisotropy = config.MaxAnisotropy;
    info.compareEnable = config.CompareEnable;
    info.compareOp = config.CompareOp;
    info.minLod = config.MinLod;
    info.maxLod = config.MaxLod;
    info.borderColor = config.BorderColor;
    info.unnormalizedCoordinates = config.UnnormalizedCoordinates;

    if (vkCreateSampler(device.LogicalDevice, &info, nullptr, &out.sampler)
        != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::SamplerCreationFailed,
                "Failed to create sampler") };
    }

    return { std::move(out), CustomError() };
}

} // namespace yst::core

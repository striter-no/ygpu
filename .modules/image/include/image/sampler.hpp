// yst sampler module
#pragma once
#include <vulkan/vulkan.h>

#include <device/device.hpp>
#include <errors.hpp>
#include <utility>

#include "config.hpp"

namespace yst::core {

class Sampler {
public:
    VkSampler sampler = VK_NULL_HANDLE;

    Sampler() = default;
    ~Sampler() { Destroy(); }

    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;
    Sampler(Sampler&& other) noexcept;
    Sampler& operator=(Sampler&& other) noexcept;

    void Destroy();
    bool IsValid() const noexcept { return sampler != VK_NULL_HANDLE; }

private:
    Device* device_ = nullptr;
    friend std::pair<Sampler, CustomError> CreateSampler(
        Device& device, const SamplerConfig& config);
};

std::pair<Sampler, CustomError> CreateSampler(
    Device& device, const SamplerConfig& config);

/// Convenience overload: linear filtering + repeat addressing.
inline std::pair<Sampler, CustomError> CreateSampler(Device& device)
{
    return CreateSampler(device, CreateConfig(SamplerPreset::LinearRepeat));
}

} // namespace yst::core

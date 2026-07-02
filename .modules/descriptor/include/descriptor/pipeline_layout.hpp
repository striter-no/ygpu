#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include <descriptor/bind_group_layout.hpp>
#include <device/device.hpp>
#include <errors.hpp>

namespace yst::core {

struct PipelineLayoutConfig {
    std::vector<BindGroupLayout*> BindGroupLayouts;
    std::vector<VkPushConstantRange> PushConstantRanges;
    VkPipelineLayoutCreateFlags Flags = 0;
};

/// Owning wrapper around a VkPipelineLayout. RAII.
class PipelineLayout {
public:
    VkPipelineLayout layout = VK_NULL_HANDLE;
    PipelineLayoutConfig config;

    PipelineLayout() = default;
    ~PipelineLayout() { Destroy(); }

    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;
    PipelineLayout(PipelineLayout&& other) noexcept;
    PipelineLayout& operator=(PipelineLayout&& other) noexcept;

    void Destroy();

private:
    Device* device_ = nullptr;

    friend std::pair<PipelineLayout, CustomError> CreatePipelineLayout(
        Device& device, const PipelineLayoutConfig& config);
};

std::pair<PipelineLayout, CustomError> CreatePipelineLayout(
    Device& device, const PipelineLayoutConfig& config);

} // namespace yst::core

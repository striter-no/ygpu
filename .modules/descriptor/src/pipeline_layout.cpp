#include <descriptor/pipeline_layout.hpp>

namespace yst::core {

PipelineLayoutConfig CreateConfig(PipelineLayoutPreset preset)
{
    PipelineLayoutConfig cfg;
    switch (preset) {
    case PipelineLayoutPreset::Empty:
    default:
        return cfg;
    }
}

PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
{
    device_ = other.device_;
    layout = other.layout;
    config = std::move(other.config);
    other.device_ = nullptr;
    other.layout = VK_NULL_HANDLE;
}

PipelineLayout& PipelineLayout::operator=(PipelineLayout&& other) noexcept
{
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        layout = other.layout;
        config = std::move(other.config);
        other.device_ = nullptr;
        other.layout = VK_NULL_HANDLE;
    }
    return *this;
}

void PipelineLayout::Destroy()
{
    if (layout != VK_NULL_HANDLE && device_) {
        vkDestroyPipelineLayout(device_->LogicalDevice, layout, nullptr);
        layout = VK_NULL_HANDLE;
        device_ = nullptr;
    }
}

std::pair<PipelineLayout, CustomError> CreatePipelineLayout(
    Device& device, const PipelineLayoutConfig& config)
{
    PipelineLayout out;
    out.device_ = &device;
    out.config = config;

    std::vector<VkDescriptorSetLayout> layouts;
    layouts.reserve(config.BindGroupLayouts.size());
    for (const auto* bgl : config.BindGroupLayouts) {
        if (bgl && bgl->layout != VK_NULL_HANDLE)
            layouts.push_back(bgl->layout);
    }

    VkPipelineLayoutCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.flags = config.Flags;
    info.setLayoutCount = static_cast<uint32_t>(layouts.size());
    info.pSetLayouts = layouts.data();
    info.pushConstantRangeCount = static_cast<uint32_t>(config.PushConstantRanges.size());
    info.pPushConstantRanges = config.PushConstantRanges.data();

    if (vkCreatePipelineLayout(device.LogicalDevice, &info, nullptr, &out.layout)
        != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::PipelineLayoutCreationFailed,
                "Failed to create PipelineLayout") };
    }

    return { std::move(out), CustomError() };
}

} // namespace yst::core


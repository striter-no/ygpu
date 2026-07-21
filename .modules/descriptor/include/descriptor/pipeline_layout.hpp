#pragma once
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <descriptor/bind_group_layout.hpp>
#include <device/device.hpp>
#include <errors.hpp>

namespace yst::core {

enum class PipelineLayoutPreset {
    Empty = 0,
};

struct PipelineLayoutConfig {
    std::vector<const BindGroupLayout*>
        BindGroupLayouts;

    std::vector<VkPushConstantRange>
        PushConstantRanges;

    VkPipelineLayoutCreateFlags Flags = 0;
};

PipelineLayoutConfig CreateConfig(PipelineLayoutPreset preset);

class PipelineLayoutBuilder {
public:
    PipelineLayoutBuilder() = default;
    explicit PipelineLayoutBuilder(PipelineLayoutPreset preset)
        : cfg_(CreateConfig(preset))
    {
    }
    explicit PipelineLayoutBuilder(PipelineLayoutConfig config)
        : cfg_(std::move(config))
    {
    }

    PipelineLayoutBuilder& AddBindGroupLayout(const BindGroupLayout& layout)
    {
        cfg_.BindGroupLayouts.push_back(
            &layout);

        return *this;
    }

    PipelineLayoutBuilder& AddPushConstantRange(const VkPushConstantRange& range)
    {
        cfg_.PushConstantRanges.push_back(range);
        return *this;
    }
    PipelineLayoutBuilder& WithFlags(VkPipelineLayoutCreateFlags flags)
    {
        cfg_.Flags = flags;
        return *this;
    }

    PipelineLayoutConfig Build() const { return cfg_; }

private:
    PipelineLayoutConfig cfg_;
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

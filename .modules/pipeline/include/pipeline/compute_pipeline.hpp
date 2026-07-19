#pragma once
#include <device/device.hpp>
#include <errors.hpp>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace yst::core {

struct ComputePipelineConfig {
    VkPipelineLayout Layout = VK_NULL_HANDLE;
    std::vector<uint32_t> Spirv;
    std::string EntryPoint = "main";
    VkPipelineCreateFlags Flags = 0;
};

class ComputePipelineBuilder {
public:
    ComputePipelineBuilder() = default;

    ComputePipelineBuilder& WithPipelineLayout(VkPipelineLayout layout)
    {
        cfg_.Layout = layout;
        return *this;
    }

    ComputePipelineBuilder& WithShaderSpv(std::vector<uint32_t> spv)
    {
        cfg_.Spirv = std::move(spv);
        return *this;
    }

    ComputePipelineBuilder& WithEntryPoint(std::string name)
    {
        cfg_.EntryPoint = std::move(name);
        return *this;
    }

    ComputePipelineConfig Build() const { return cfg_; }

private:
    ComputePipelineConfig cfg_;
};

class ComputePipeline {
public:
    VkPipeline pipeline = VK_NULL_HANDLE;

    ComputePipeline() = default;
    ~ComputePipeline() { Destroy(); }

    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;
    ComputePipeline(ComputePipeline&& other) noexcept
    {
        device_ = other.device_;
        pipeline = other.pipeline;
        other.device_ = nullptr;
        other.pipeline = VK_NULL_HANDLE;
    }
    ComputePipeline& operator=(ComputePipeline&& other) noexcept
    {
        if (this != &other) {
            Destroy();
            device_ = other.device_;
            pipeline = other.pipeline;
            other.device_ = nullptr;
            other.pipeline = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Destroy()
    {
        if (pipeline != VK_NULL_HANDLE && device_) {
            vkDestroyPipeline(device_->LogicalDevice, pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
            device_ = nullptr;
        }
    }

private:
    Device* device_ = nullptr;
    friend std::pair<ComputePipeline, CustomError> CreateComputePipeline(
        Device& device, const ComputePipelineConfig& config);
};

std::pair<ComputePipeline, CustomError> CreateComputePipeline(
    Device& device, const ComputePipelineConfig& config);

} // namespace yst::core

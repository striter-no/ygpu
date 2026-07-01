#pragma once
#include <vulkan/vulkan.h>

#include <device/device.hpp>
#include <vector>

namespace yst::core {

struct GraphicsPipeline {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    void Destroy(Device& device);
};

struct PipelineConfig {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    std::vector<uint32_t> vertexShaderSpv;
    std::vector<uint32_t> fragmentShaderSpv;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};

std::pair<GraphicsPipeline, CustomError> CreateGraphicsPipeline(
    Device& device, const PipelineConfig& config);

} // namespace yst::core

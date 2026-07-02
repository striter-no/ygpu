#pragma once
#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace yst::core {

/// Layer 2 configuration for a VkShaderModule.
struct ShaderModuleConfig {
    VkShaderStageFlagBits Stage = VK_SHADER_STAGE_VERTEX_BIT;
    std::vector<uint32_t> Spirv;
    std::string EntryPoint = "main";
};

} // namespace yst::core

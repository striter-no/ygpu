// yst shader module — Builder pattern (v4)
#pragma once

#include <string>
#include <utility>
#include <vector>

#include <device/device.hpp>
#include <errors.hpp>
#include <vulkan/vulkan_core.h>

#include "config.hpp"

namespace yst::core {

class ShaderModule {
public:
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT;
    std::string entryPoint = "main";

    ShaderModule() = default;
    ~ShaderModule() { Destroy(); }

    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;
    ShaderModule(ShaderModule&& other) noexcept;
    ShaderModule& operator=(ShaderModule&& other) noexcept;

    void Destroy();
    VkPipelineShaderStageCreateInfo ToStageCreateInfo() const noexcept;

private:
    Device* device_ = nullptr;
    friend std::pair<ShaderModule, CustomError> CreateShaderModule(
        Device& device, const ShaderModuleConfig& config);
};

std::pair<ShaderModule, CustomError> CreateShaderModule(
    Device& device, const ShaderModuleConfig& config);

std::pair<std::vector<uint32_t>, CustomError> LoadSpvFile(
    const std::string& path);

std::pair<std::vector<uint32_t>, CustomError> LoadAndCompileGlslFile(
    const std::string& path, VkShaderStageFlagBits stage);

} // namespace yst::core

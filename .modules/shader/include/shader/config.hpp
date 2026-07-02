// yst shader module config
#pragma once

#include <string>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace yst::core {

class ShaderModule;

enum class ShaderModulePreset {
    Vertex = 0,
    Fragment,
    Compute,
};

struct ShaderModuleConfig {
    VkShaderStageFlagBits Stage = VK_SHADER_STAGE_VERTEX_BIT;
    std::vector<uint32_t> Spirv;
    std::string EntryPoint = "main";
};

ShaderModuleConfig CreateConfig(ShaderModulePreset preset);

class ShaderModuleBuilder {
public:
    ShaderModuleBuilder() = default;
    explicit ShaderModuleBuilder(ShaderModulePreset preset)
        : cfg_(CreateConfig(preset))
    {
    }
    explicit ShaderModuleBuilder(ShaderModuleConfig config)
        : cfg_(std::move(config))
    {
    }

    ShaderModuleBuilder& WithStage(VkShaderStageFlagBits stage)
    {
        cfg_.Stage = stage;
        return *this;
    }
    ShaderModuleBuilder& WithSpirv(std::vector<uint32_t> spv)
    {
        cfg_.Spirv = std::move(spv);
        return *this;
    }
    ShaderModuleBuilder& WithEntryPoint(std::string name)
    {
        cfg_.EntryPoint = std::move(name);
        return *this;
    }

    ShaderModuleConfig Build() const { return cfg_; }

private:
    ShaderModuleConfig cfg_;
};

} // namespace yst::core

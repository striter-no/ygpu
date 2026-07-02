// yst shader module implementation — Builder pattern (v3)
#include <shader/shader.hpp>

#include <cstring>
#include <fstream>

namespace yst::core {

ShaderModuleConfig CreateConfig(ShaderModulePreset preset)
{
    ShaderModuleConfig cfg;

    switch (preset) {
    case ShaderModulePreset::Fragment:
        cfg.Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        return cfg;

    case ShaderModulePreset::Compute:
        cfg.Stage = VK_SHADER_STAGE_COMPUTE_BIT;
        return cfg;

    case ShaderModulePreset::Vertex:
    default:
        cfg.Stage = VK_SHADER_STAGE_VERTEX_BIT;
        return cfg;
    }
}

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
{
    device_ = other.device_;
    module = other.module;
    stage = other.stage;
    entryPoint = std::move(other.entryPoint);

    other.device_ = nullptr;
    other.module = VK_NULL_HANDLE;
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept
{
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        module = other.module;
        stage = other.stage;
        entryPoint = std::move(other.entryPoint);

        other.device_ = nullptr;
        other.module = VK_NULL_HANDLE;
    }
    return *this;
}

void ShaderModule::Destroy()
{
    if (module != VK_NULL_HANDLE && device_) {
        vkDestroyShaderModule(device_->LogicalDevice, module, nullptr);
        module = VK_NULL_HANDLE;
        device_ = nullptr;
    }
}

VkPipelineShaderStageCreateInfo ShaderModule::ToStageCreateInfo() const noexcept
{
    VkPipelineShaderStageCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = stage;
    info.module = module;
    info.pName = entryPoint.c_str();
    info.pSpecializationInfo = nullptr;
    return info;
}

std::pair<ShaderModule, CustomError> CreateShaderModule(
    Device& device, const ShaderModuleConfig& config)
{
    ShaderModule out;
    out.device_ = &device;
    out.stage = config.Stage;
    out.entryPoint = config.EntryPoint;

    if (config.Spirv.empty()) {
        return { std::move(out),
            CustomError(ErrorCode::ShaderFileEmpty,
                "Cannot create shader module from empty SPIR-V") };
    }

    if (config.Spirv.size() * sizeof(uint32_t) % 4 != 0) {
        return { std::move(out),
            CustomError(ErrorCode::ShaderFileEmpty,
                "SPIR-V size is not a multiple of 4 bytes") };
    }

    VkShaderModuleCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = config.Spirv.size() * sizeof(uint32_t);
    info.pCode = config.Spirv.data();

    if (vkCreateShaderModule(device.LogicalDevice, &info, nullptr, &out.module)
        != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::ShaderModuleCreationFailed,
                "Failed to create shader module") };
    }

    return { std::move(out), CustomError() };
}

std::pair<std::vector<uint32_t>, CustomError> LoadSpvFile(
    const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return { {},
            CustomError(ErrorCode::ShaderFileOpenFailed,
                "Failed to open shader: " + path) };
    }

    const std::streamoff fileSize = file.tellg();
    if (fileSize <= 0) {
        return { {},
            CustomError(ErrorCode::ShaderFileEmpty,
                "Shader file is empty: " + path) };
    }
    if (fileSize % sizeof(uint32_t) != 0) {
        return { {},
            CustomError(ErrorCode::ShaderFileEmpty,
                "Shader file size is not a multiple of 4 bytes: " + path) };
    }

    std::vector<uint32_t> buffer(static_cast<size_t>(fileSize) / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    return { std::move(buffer), CustomError() };
}

} // namespace yst::core


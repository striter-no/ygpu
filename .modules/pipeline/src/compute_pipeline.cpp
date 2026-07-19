#include <pipeline/compute_pipeline.hpp>

namespace yst::core {

std::pair<ComputePipeline, CustomError> CreateComputePipeline(
    Device& device, const ComputePipelineConfig& config)
{
    ComputePipeline out;
    out.device_ = &device;

    if (config.Layout == VK_NULL_HANDLE) {
        return { std::move(out), CustomError(ErrorCode::ComputePipelineCreationFailed, "ComputePipeline requires a valid PipelineLayout") };
    }

    // Создаем временный VkShaderModule
    VkShaderModuleCreateInfo moduleInfo {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = config.Spirv.size() * sizeof(uint32_t);
    moduleInfo.pCode = config.Spirv.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device.LogicalDevice, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return { std::move(out), CustomError(ErrorCode::ShaderModuleCreationFailed, "Failed to create shader module for compute pipeline") };
    }

    VkComputePipelineCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.flags = config.Flags;
    info.layout = config.Layout;
    info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    info.stage.module = shaderModule;
    info.stage.pName = config.EntryPoint.c_str();

    if (vkCreateComputePipelines(device.LogicalDevice, VK_NULL_HANDLE, 1, &info, nullptr, &out.pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(device.LogicalDevice, shaderModule, nullptr);
        return { std::move(out), CustomError(ErrorCode::ComputePipelineCreationFailed, "Failed to create compute pipeline") };
    }

    // Модуль можно удалять сразу после создания pipeline
    vkDestroyShaderModule(device.LogicalDevice, shaderModule, nullptr);

    return { std::move(out), CustomError() };
}

} // namespace yst::core

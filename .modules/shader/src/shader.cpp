// yst shader module implementation — Builder pattern (v3)
#include <cstdio>
#include <fstream>
#include <iostream>
#include <shader/shader.hpp>
#include <sstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace yst::core {

static bool FileExists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

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

std::pair<std::vector<uint32_t>, CustomError> LoadAndCompileGlslFile(
    const std::string& path, VkShaderStageFlagBits stage)
{
    if (!FileExists(path)) {
        return { {}, CustomError(ErrorCode::ShaderFileOpenFailed, "Shader file does not exist: " + path) };
    }

    std::string stageStr;
    switch (stage) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        stageStr = "vertex";
        break;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        stageStr = "fragment";
        break;
    case VK_SHADER_STAGE_COMPUTE_BIT:
        stageStr = "compute";
        break;
    default:
        return { {}, CustomError(ErrorCode::ShaderModuleCreationFailed, "Unsupported shader stage") };
    }

    char tempFileTemplate[] = "/tmp/yst_shader_XXXXXX.spv";
    int fd = mkstemps(tempFileTemplate, 4);
    if (fd == -1) {
        return { {}, CustomError(ErrorCode::ShaderModuleCreationFailed, "Failed to create temp file") };
    }
    close(fd);
    std::string tempFile = tempFileTemplate;
    std::string stageArg = "-fshader-stage=" + stageStr;

    pid_t pid = fork();
    if (pid == 0) {
        char* args[] = {
            const_cast<char*>("glslc"),
            const_cast<char*>(stageArg.c_str()),
            const_cast<char*>("-o"),
            const_cast<char*>(tempFile.c_str()),
            const_cast<char*>(path.c_str()),
            nullptr
        };

        execvp("glslc", args);

        std::cerr << "[Shader] Failed to execute glslc. Is it in PATH?" << std::endl;
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            std::remove(tempFile.c_str());
            return { {}, CustomError(ErrorCode::ShaderModuleCreationFailed, "glslc failed to compile shader (check console output for errors): " + path) };
        }
    } else {
        std::remove(tempFile.c_str());
        return { {}, CustomError(ErrorCode::ShaderModuleCreationFailed, "Failed to fork process") };
    }

    auto [spv, err] = LoadSpvFile(tempFile);
    std::remove(tempFile.c_str());
    return { std::move(spv), err };
}

} // namespace yst::core

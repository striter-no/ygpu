#include <enums.hpp>
#include <errors.hpp>

#include <cstdio>

namespace yst {

const char* ErrorName(int code)
{
    switch (static_cast<ErrorCode>(code)) {
    case ErrorCode::None:
        return "None";
    case ErrorCode::Unknown:
        return "Unknown";

    // Window
    case ErrorCode::WindowGLFWInitFailed:
        return "WindowGLFWInitFailed";
    case ErrorCode::WindowCreationFailed:
        return "WindowCreationFailed";

    // Device
    case ErrorCode::InstanceCreationFailed:
        return "InstanceCreationFailed";
    case ErrorCode::PhysicalDeviceSelectionFailed:
        return "PhysicalDeviceSelectionFailed";
    case ErrorCode::LogicalDeviceCreationFailed:
        return "LogicalDeviceCreationFailed";
    case ErrorCode::GraphicsQueueNotFound:
        return "GraphicsQueueNotFound";
    case ErrorCode::VmaAllocatorCreationFailed:
        return "VmaAllocatorCreationFailed";
    case ErrorCode::UnsupportedBackend:
        return "UnsupportedBackend";
    case ErrorCode::MissingRequiredDeviceExtension:
        return "MissingRequiredDeviceExtension";

    // Swapchain
    case ErrorCode::SurfaceCreationFailed:
        return "SurfaceCreationFailed";
    case ErrorCode::PresentNotSupported:
        return "PresentNotSupported";
    case ErrorCode::SwapchainBuildFailed:
        return "SwapchainBuildFailed";
    case ErrorCode::SwapchainImageAcquireFailed:
        return "SwapchainImageAcquireFailed";
    case ErrorCode::SwapchainPresentFailed:
        return "SwapchainPresentFailed";
    case ErrorCode::SwapchainOutOfDate:
        return "SwapchainOutOfDate";
    case ErrorCode::SwapchainSuboptimal:
        return "SwapchainSuboptimal";
    case ErrorCode::SwapchainRebuildFailed:
        return "SwapchainRebuildFailed";
    case ErrorCode::RenderPassCreationFailed:
        return "RenderPassCreationFailed";
    case ErrorCode::FramebufferCreationFailed:
        return "FramebufferCreationFailed";
    case ErrorCode::SyncObjectCreationFailed:
        return "SyncObjectCreationFailed";

    // Command
    case ErrorCode::CommandPoolCreationFailed:
        return "CommandPoolCreationFailed";
    case ErrorCode::CommandBufferAllocationFailed:
        return "CommandBufferAllocationFailed";

    // Buffer
    case ErrorCode::BufferCreationFailed:
        return "BufferCreationFailed";
    case ErrorCode::BufferNotHostVisible:
        return "BufferNotHostVisible";
    case ErrorCode::BufferSizeExceeded:
        return "BufferSizeExceeded";

    // Pipeline
    case ErrorCode::ShaderModuleCreationFailed:
        return "ShaderModuleCreationFailed";
    case ErrorCode::PipelineLayoutCreationFailed:
        return "PipelineLayoutCreationFailed";
    case ErrorCode::GraphicsPipelineCreationFailed:
        return "GraphicsPipelineCreationFailed";

    // Shader
    case ErrorCode::ShaderFileOpenFailed:
        return "ShaderFileOpenFailed";
    case ErrorCode::ShaderFileEmpty:
        return "ShaderFileEmpty";
    }

    // Fall back: Vulkan VkResult values are negative; surface them by number.
    static thread_local char buf[32];
    if (code < 0) {
        std::snprintf(buf, sizeof(buf), "VkResult(%d)", code);
        return buf;
    }
    std::snprintf(buf, sizeof(buf), "ErrorCode(%d)", code);
    return buf;
}

std::string CustomError::str() const
{
    if (code == 0)
        return "[no errors]";

    return std::string("[") + ErrorName(code) + "] " + message;
}

} // namespace yst

#pragma once

#include <vulkan/vulkan_core.h>

namespace yst {

/// Centralised error code registry.
///
/// All engine subsystems return CustomError values whose `code` field is one
/// of these (or, for the rare cases where we forward a Vulkan VkResult
/// directly, the raw negative VkResult value).
///
/// Numeric ranges:
///   Window          100-199
///   Device          200-299
///   Swapchain       300-399
///   Command         400-499
///   Buffer          500-599
///   Pipeline        600-699
///   Shader          700-799
///
/// The values intentionally do not start at 0 (0 == no error) and do not
/// collide with positive Vulkan VkResult values, so a single `int code`
/// field can carry either an ErrorCode or a raw VkResult without ambiguity.
enum class ErrorCode : int {
    None = 0,
    Unknown = -1,

    // ---- Window (yst::ywin) -------------------------------------------
    WindowGLFWInitFailed = 100,
    WindowCreationFailed = 101,

    // ---- Device (yst::core) -------------------------------------------
    InstanceCreationFailed = 200,
    PhysicalDeviceSelectionFailed = 201,
    LogicalDeviceCreationFailed = 202,
    GraphicsQueueNotFound = 203,
    VmaAllocatorCreationFailed = 204,
    UnsupportedBackend = 205,
    MissingRequiredDeviceExtension = 206,

    // ---- Swapchain (yst::core) ----------------------------------------
    SurfaceCreationFailed = 300,
    PresentNotSupported = 301,
    SwapchainBuildFailed = 302,
    SwapchainImageAcquireFailed = 303,
    SwapchainPresentFailed = 304,
    SwapchainOutOfDate = 305,
    SwapchainSuboptimal = 306,
    SwapchainRebuildFailed = 307,
    RenderPassCreationFailed = 308,
    FramebufferCreationFailed = 309,
    SyncObjectCreationFailed = 310,

    // ---- Command (yst::core) ------------------------------------------
    CommandPoolCreationFailed = 400,
    CommandBufferAllocationFailed = 401,

    // ---- Buffer (yst::core) -------------------------------------------
    BufferCreationFailed = 500,
    BufferNotHostVisible = 501,
    BufferSizeExceeded = 502,
    BufferUploadFailed = 503,
    BufferDownloadFailed = 504,

    // ---- Pipeline (yst::core) -----------------------------------------
    ShaderModuleCreationFailed = 600,
    PipelineLayoutCreationFailed = 601,
    GraphicsPipelineCreationFailed = 602,
    ComputePipelineCreationFailed = 603,

    // ---- Shader (yst::core) -------------------------------------------
    ShaderFileOpenFailed = 700,
    ShaderFileEmpty = 701,

    // ---- Descriptor (yst::core) ---------------------------------------
    DescriptorPoolCreationFailed = 800,
    DescriptorPoolExhausted = 801,
    DescriptorSetLayoutCreationFailed = 802,
    DescriptorSetAllocationFailed = 803,
    DescriptorSetUpdateFailed = 804,

    // ---- Image (yst::core) --------------------------------------------
    ImageCreationFailed = 900,
    ImageViewCreationFailed = 901,
    SamplerCreationFailed = 902,

    // ---- Texture (yst::core) ------------------------------------------
    TextureFileOpenFailed = 1000,
    TextureFileDecodeFailed = 1001,
    TextureUploadFailed = 1002,
    TextureUnsupportedFormat = 1003,
};

/// Convert an ErrorCode (or raw int) to a short symbolic name, e.g.
/// "SwapchainOutOfDate" or "VkResult(-4)" for raw Vulkan codes.
/// Used by CustomError::str() for diagnostics.
const char* ErrorName(int code);

} // namespace yst

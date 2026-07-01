#pragma once
#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace yst::core {

/// Render-pass attachment description for the swapchain's color image.
/// Defaults match the historical hardcoded behaviour (CLEAR on load,
/// STORE on write, transition to PRESENT_SRC_KHR at the end).
struct ColorAttachmentDesc {
    VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentLoadOp StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

/// Optional depth/stencil attachment. If `Format` is VK_FORMAT_UNDEFINED
/// (the default), no depth attachment is created. Otherwise a second
/// attachment is added to the render pass and an extra framebuffer
/// attachment is bound.
struct DepthAttachmentDesc {
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentLoadOp StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout FinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /// Depth/stencil clear values used when LoadOp == CLEAR.
    VkClearDepthStencilValue ClearValue { 1.0f, 0 };
};

/// Subpass dependency between EXTERNAL and the first subpass. Defaults match
/// the historical masks (color-attachment-output / color-attachment-write).
struct SubpassDependencyDesc {
    VkPipelineStageFlags SrcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkAccessFlags SrcAccessMask = 0;
    VkPipelineStageFlags DstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkAccessFlags DstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
};

/// Layer 2 configuration for a swapchain + its implicit render pass.
///
/// Defaults reproduce the historical hardcoded swapchain. Callers can opt
/// into depth buffering by setting Depth.Format to a depth format and
/// supplying a depth image view via DepthImageViewOverride (or by letting
/// the swapchain allocate one — not yet implemented, see TODO).
struct SwapchainConfig {
    uint32_t MaxFramesInFlight = 2;

    // Present / format selection
    VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    std::vector<VkPresentModeKHR> FallbackPresentModes; // tried in order if desired fails.
    VkFormat PreferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR PreferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    // Render-pass attachment descriptions
    ColorAttachmentDesc Color;
    DepthAttachmentDesc Depth;
    SubpassDependencyDesc ExternalDependency;

    /// Clear color used when Color.LoadOp == CLEAR.
    float ClearColor[4] = { 0.05f, 0.05f, 0.2f, 1.0f };

    /// Caller-supplied depth image view. If non-null and Depth.Format is
    /// set, this view is attached as the second framebuffer attachment.
    /// If null and Depth.Format is set, CreateSwapchain returns
    /// SwapchainBuildFailed with a descriptive message.
    VkImageView DepthImageViewOverride = VK_NULL_HANDLE;
};

} // namespace yst::core

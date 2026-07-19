#pragma once
#include <vulkan/vulkan.h>

#include <cstdint>
#include <functional>

#include <descriptor/bind_group.hpp>
#include <descriptor/pipeline_layout.hpp>
#include <device/device.hpp>
#include <errors.hpp>

namespace yst::core {

enum class CommandPoolPreset {
    Resettable = 0,
    Transient,
};

/// Layer 2 configuration for a VkCommandPool.
struct CommandPoolConfig {
    VkCommandPoolCreateFlags Flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    uint32_t QueueFamilyIndex = 0; ///< Caller must fill this in.
};

CommandPoolConfig CreateConfig(CommandPoolPreset preset);

/// Bind point selector for command buffer methods that historically only
/// supported graphics.
enum class PipelineBindPoint : uint32_t {
    Graphics = VK_PIPELINE_BIND_POINT_GRAPHICS,
    Compute = VK_PIPELINE_BIND_POINT_COMPUTE,
    RayTracing = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
};

class CommandList {
public:
    VkCommandBuffer buffer = VK_NULL_HANDLE;
    uint32_t imageIndex = 0;

    /// Begin recording with explicit flags. Defaults to the historical
    /// ONE_TIME_SUBMIT_BIT, which is correct for per-frame command buffers
    /// that are recorded and submitted in the same frame.
    void Begin(VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    void End();
    void Reset();

    void BindPipeline(VkPipeline pipeline,
        PipelineBindPoint bindPoint = PipelineBindPoint::Graphics);

    void BindVertexBuffer(VkBuffer vbuffer, VkDeviceSize offset = 0);
    void BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount,
        const VkBuffer* buffers, const VkDeviceSize* offsets);

    void BindIndexBuffer(VkBuffer indexBuffer, VkDeviceSize offset = 0,
        VkIndexType indexType = VK_INDEX_TYPE_UINT32);

    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1,
        uint32_t firstVertex = 0, uint32_t firstInstance = 0);

    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
        uint32_t firstIndex = 0, int32_t vertexOffset = 0,
        uint32_t firstInstance = 0);

    void BindDescriptorSets(VkPipelineLayout layout, PipelineBindPoint bindPoint,
        uint32_t firstSet, uint32_t descriptorSetCount,
        const VkDescriptorSet* pDescriptorSets,
        uint32_t dynamicOffsetCount = 0,
        const uint32_t* pDynamicOffsets = nullptr);

    /// Convenience (WebGPU-style): bind a single BindGroup at set index
    /// `setIndex` to the given pipeline layout. Uses the graphics bind
    /// point by default.
    void BindBindGroup(VkPipelineLayout layout, uint32_t setIndex,
        const BindGroup& bg, PipelineBindPoint bindPoint = PipelineBindPoint::Graphics)
    {
        BindDescriptorSets(layout, bindPoint, setIndex, 1, &bg.set, 0, nullptr);
    }

    /// Begin a render pass with a single clear value (color-only).
    /// Backwards-compatible with the historical signature.
    void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer,
        VkExtent2D extent, const float clearColor[4],
        VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

    /// Begin a render pass with arbitrary clear values (color + depth/stencil).
    void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer,
        VkExtent2D extent, uint32_t clearValueCount,
        const VkClearValue* pClearValues,
        VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

    void EndRenderPass();

    // ---- Image barrier / layout transitions ---------------------------
    //
    // These are recording-only operations (they emit commands into the
    // command buffer). They do NOT submit anything. Callers are expected
    // to submit the command buffer (e.g. via a one-time-submit helper) for
    // the transitions to take effect.

    /// Transition a single image's layout using an execution + memory
    /// barrier with the given stage masks. This is the lowest-level
    /// layout-transition API — use TransitionImageLayout() for the common
    /// case where the stage masks can be inferred.
    void PipelineBarrierImage(VkImage image,
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        uint32_t baseMipLevel = 0, uint32_t levelCount = VK_REMAINING_MIP_LEVELS,
        uint32_t baseArrayLayer = 0, uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS);

    /// Convenience: transition an image between common layouts. Picks
    /// reasonable stage/access masks for each layout pair. Use the lower-
    /// level PipelineBarrierImage() if you need custom masks.
    void TransitionImageLayout(VkImage image,
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        uint32_t mipLevels = 1, uint32_t arrayLayers = 1);

    /// Copy a region of a buffer into an image. The image should be in
    /// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL before this call (use
    /// TransitionImageLayout() to put it there).
    void CopyBufferToImage(VkBuffer buffer, VkImage image,
        uint32_t width, uint32_t height,
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

    /// Blit one mip level of a source image into the next mip level of a
    /// destination image. Used for mipmap generation. Both images must be
    /// in TRANSFER_SRC_OPTIMAL / TRANSFER_DST_OPTIMAL layouts respectively.
    void BlitImageMip(VkImage srcImage, VkImage dstImage,
        uint32_t srcWidth, uint32_t srcHeight, uint32_t srcMip,
        uint32_t dstWidth, uint32_t dstHeight, uint32_t dstMip,
        VkFilter filter = VK_FILTER_LINEAR,
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void PipelineBarrierBuffer(VkBuffer buffer,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
};

std::pair<VkCommandPool, CustomError> CreateCommandPool(
    Device& device, const CommandPoolConfig& config);

/// Convenience overload (Layer 1): historical signature — pool for the
/// device's graphics queue family with RESET_COMMAND_BUFFER_BIT.
std::pair<VkCommandPool, CustomError> CreateCommandPool(Device& device);

std::pair<CommandList, CustomError> AllocateCommandList(Device& device,
    VkCommandPool pool,
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

/// Synchronous one-time-submit helper. Allocates a command buffer from a
/// temporary pool, runs `recorder(cmd)` to record into it, submits and
/// waits for completion, then frees everything. Returns the recorder's
/// error or a submission failure.
///
/// Usage:
///   auto err = SubmitOneTimeCommands(device, [](CommandList& cmd) {
///       cmd.Begin();
///       cmd.TransitionImageLayout(img, ...);
///       cmd.CopyBufferToImage(staging, img, ...);
///       cmd.End();
///       return CustomError();
///   });
CustomError SubmitOneTimeCommands(Device& device,
    const std::function<CustomError(CommandList&)>& recorder);

} // namespace yst::core

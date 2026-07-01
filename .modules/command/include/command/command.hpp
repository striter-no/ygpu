#pragma once
#include <vulkan/vulkan.h>

#include <device/device.hpp>
#include <errors.hpp>

#include <cstdint>
#include <vector>

namespace yst::core {

/// Layer 2 configuration for a VkCommandPool.
struct CommandPoolConfig {
    VkCommandPoolCreateFlags Flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    uint32_t QueueFamilyIndex = 0; ///< Caller must fill this in.
};

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
};

std::pair<VkCommandPool, CustomError> CreateCommandPool(
    Device& device, const CommandPoolConfig& config);

/// Convenience overload (Layer 1): historical signature — pool for the
/// device's graphics queue family with RESET_COMMAND_BUFFER_BIT.
std::pair<VkCommandPool, CustomError> CreateCommandPool(Device& device);

std::pair<CommandList, CustomError> AllocateCommandList(Device& device,
    VkCommandPool pool,
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

} // namespace yst::core

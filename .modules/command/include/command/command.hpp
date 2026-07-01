#pragma once
#include <vulkan/vulkan.h>

#include <device/device.hpp>
#include <errors.hpp>

namespace yst::core {

class CommandList {
public:
    VkCommandBuffer buffer = VK_NULL_HANDLE;
    uint32_t imageIndex = 0;

    void Begin();
    void End();
    void Reset();

    void BindPipeline(VkPipeline pipeline);
    void BindVertexBuffer(VkBuffer buffer);
    void Draw(size_t vertexCount);

    void BindIndexBuffer(VkBuffer buffer, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void DrawIndexed(size_t indexCount);

    void BindDescriptorSets(VkPipelineLayout layout, uint32_t firstSet,
        uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets);

    void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer,
        VkExtent2D extent, const float clearColor[4]);
    void EndRenderPass();
};

std::pair<VkCommandPool, CustomError> CreateCommandPool(Device& device);
std::pair<CommandList, CustomError> AllocateCommandList(Device& device,
    VkCommandPool pool);

} // namespace yst::core

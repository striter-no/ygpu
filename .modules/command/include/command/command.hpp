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
    void Draw(uint32_t vertexCount);

    void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer,
                         VkExtent2D extent, const float clearColor[4]);
    void EndRenderPass();
};

std::pair<VkCommandPool, CustomError> CreateCommandPool(Device& device);
std::pair<CommandList, CustomError> AllocateCommandList(Device& device,
                                                        VkCommandPool pool);

}  // namespace yst::core

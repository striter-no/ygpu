#include <command/command.hpp>

#include <cstring>

namespace yst::core {

void CommandList::Begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;
    vkBeginCommandBuffer(buffer, &beginInfo);
}

void CommandList::End() { vkEndCommandBuffer(buffer); }

void CommandList::Reset() { vkResetCommandBuffer(buffer, 0); }

void CommandList::BindPipeline(VkPipeline pipeline, PipelineBindPoint bindPoint)
{
    vkCmdBindPipeline(buffer, static_cast<VkPipelineBindPoint>(bindPoint),
        pipeline);
}

void CommandList::BindVertexBuffer(VkBuffer vbuffer, VkDeviceSize offset)
{
    VkDeviceSize offsets[] = { offset };
    vkCmdBindVertexBuffers(buffer, 0, 1, &vbuffer, offsets);
}

void CommandList::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount,
    const VkBuffer* buffers, const VkDeviceSize* offsets)
{
    vkCmdBindVertexBuffers(buffer, firstBinding, bindingCount, buffers, offsets);
}

void CommandList::BindIndexBuffer(VkBuffer indexBuffer, VkDeviceSize offset,
    VkIndexType indexType)
{
    vkCmdBindIndexBuffer(buffer, indexBuffer, offset, indexType);
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount,
    uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(buffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
    uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(buffer, indexCount, instanceCount, firstIndex,
        vertexOffset, firstInstance);
}

void CommandList::BindDescriptorSets(VkPipelineLayout layout,
    PipelineBindPoint bindPoint, uint32_t firstSet,
    uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets,
    uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    vkCmdBindDescriptorSets(buffer, static_cast<VkPipelineBindPoint>(bindPoint),
        layout, firstSet, descriptorSetCount, pDescriptorSets,
        dynamicOffsetCount, pDynamicOffsets);
}

void CommandList::BeginRenderPass(VkRenderPass renderPass,
    VkFramebuffer framebuffer, VkExtent2D extent, const float clearColor[4],
    VkSubpassContents contents)
{
    VkClearValue clearValue {};
    std::memcpy(clearValue.color.float32, clearColor, 4 * sizeof(float));

    BeginRenderPass(renderPass, framebuffer, extent, 1, &clearValue, contents);
}

void CommandList::BeginRenderPass(VkRenderPass renderPass,
    VkFramebuffer framebuffer, VkExtent2D extent, uint32_t clearValueCount,
    const VkClearValue* pClearValues, VkSubpassContents contents)
{
    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = clearValueCount;
    renderPassInfo.pClearValues = pClearValues;

    vkCmdBeginRenderPass(buffer, &renderPassInfo, contents);

    // Viewport + scissor are set from the extent so that simple renderers
    // don't need to call vkCmdSetViewport/Scissor manually. Renderers that
    // want a different viewport can re-issue these calls after BeginRenderPass.
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(buffer, 0, 1, &scissor);
}

void CommandList::EndRenderPass() { vkCmdEndRenderPass(buffer); }

std::pair<VkCommandPool, CustomError> CreateCommandPool(
    Device& device, const CommandPoolConfig& config)
{
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = config.Flags;
    poolInfo.queueFamilyIndex = config.QueueFamilyIndex;

    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(device.LogicalDevice, &poolInfo, nullptr, &pool)
        != VK_SUCCESS) {
        return { VK_NULL_HANDLE,
            CustomError(ErrorCode::CommandPoolCreationFailed,
                "Failed to create command pool") };
    }
    return { pool, CustomError() };
}

std::pair<VkCommandPool, CustomError> CreateCommandPool(Device& device)
{
    CommandPoolConfig cfg;
    cfg.QueueFamilyIndex = device.GraphicsQueueFamily;
    return CreateCommandPool(device, cfg);
}

std::pair<CommandList, CustomError> AllocateCommandList(Device& device,
    VkCommandPool pool, VkCommandBufferLevel level)
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    CommandList cmd;
    if (vkAllocateCommandBuffers(device.LogicalDevice, &allocInfo, &cmd.buffer)
        != VK_SUCCESS) {
        return { cmd,
            CustomError(ErrorCode::CommandBufferAllocationFailed,
                "Failed to allocate command buffer") };
    }
    return { cmd, CustomError() };
}

} // namespace yst::core

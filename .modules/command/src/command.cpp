#include <command/command.hpp>

namespace yst::core {

void CommandList::Begin()
{
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(buffer, &beginInfo);
}

void CommandList::End() { vkEndCommandBuffer(buffer); }

void CommandList::Reset() { vkResetCommandBuffer(buffer, 0); }

void CommandList::EndRenderPass() { vkCmdEndRenderPass(buffer); }

std::pair<VkCommandPool, CustomError> CreateCommandPool(Device& device)
{
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device.GraphicsQueueFamily;

    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(device.LogicalDevice, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        return { VK_NULL_HANDLE,
            CustomError(30, "Failed to create command pool") };
    }
    return { pool, CustomError() };
}

std::pair<CommandList, CustomError> AllocateCommandList(Device& device,
    VkCommandPool pool)
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    CommandList cmd;
    if (vkAllocateCommandBuffers(device.LogicalDevice, &allocInfo,
            &cmd.buffer)
        != VK_SUCCESS) {
        return { cmd, CustomError(31, "Failed to allocate command buffer") };
    }
    return { cmd, CustomError() };
}

void CommandList::BeginRenderPass(VkRenderPass renderPass,
    VkFramebuffer framebuffer, VkExtent2D extent,
    const float clearColor[4])
{
    VkClearValue clearValue = {
        { { clearColor[0], clearColor[1], clearColor[2], clearColor[3] } }
    };

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(buffer, 0, 1, &scissor);
}

void CommandList::BindPipeline(VkPipeline pipeline)
{
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void CommandList::BindVertexBuffer(VkBuffer vbuffer)
{
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(buffer, 0, 1, &vbuffer, offsets);
}

void CommandList::Draw(size_t vertexCount)
{
    vkCmdDraw(buffer, vertexCount, 1, 0, 0);
}

void CommandList::BindIndexBuffer(VkBuffer indexBuffer, VkIndexType indexType)
{
    vkCmdBindIndexBuffer(buffer, indexBuffer, 0, indexType);
}

void CommandList::DrawIndexed(size_t indexCount)
{
    vkCmdDrawIndexed(buffer, indexCount, 1, 0, 0, 0);
}

void CommandList::BindDescriptorSets(VkPipelineLayout layout, uint32_t firstSet,
    uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets)
{
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
        firstSet, descriptorSetCount, pDescriptorSets, 0, nullptr);
}

} // namespace yst::core

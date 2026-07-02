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

void CommandList::PipelineBarrierImage(VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkImageAspectFlags aspectMask,
    uint32_t baseMipLevel, uint32_t levelCount,
    uint32_t baseArrayLayer, uint32_t layerCount)
{
    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = baseMipLevel;
    barrier.subresourceRange.levelCount = levelCount;
    barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
    barrier.subresourceRange.layerCount = layerCount;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    vkCmdPipelineBarrier(buffer, srcStageMask, dstStageMask, 0, 0, nullptr,
        0, nullptr, 1, &barrier);
}

void CommandList::TransitionImageLayout(VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask, uint32_t mipLevels, uint32_t arrayLayers)
{
    // Pick reasonable stage/access masks for the common layout transitions.
    // The full table comes from the Vulkan tutorial's layout transition
    // helper; it covers the transitions that occur in typical rendering
    // pipelines (UNDEFINED -> TRANSFER_DST, TRANSFER_DST -> SHADER_READ,
    // etc.). For exotic transitions the caller should use
    // PipelineBarrierImage() directly.
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags srcAccess = 0;
    VkAccessFlags dstAccess = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
        && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        srcAccess = 0;
        dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstAccess = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
        && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // Skip the TRANSFER_DST intermediate (used when an image is created
        // already in its final layout, e.g. by clearing).
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        srcAccess = 0;
        dstAccess = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstAccess = VK_ACCESS_TRANSFER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        srcAccess = VK_ACCESS_TRANSFER_READ_BIT;
        dstAccess = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
        && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        srcAccess = 0;
        dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else {
        // Generic fallback — caller knows what they're doing; barrier with
        // matching access flags. If this is wrong, validation layers will
        // flag it.
        srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        srcAccess = VK_ACCESS_MEMORY_WRITE_BIT;
        dstAccess = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    }

    PipelineBarrierImage(image, oldLayout, newLayout, srcStage, dstStage,
        srcAccess, dstAccess, aspectMask, 0,
        VK_REMAINING_MIP_LEVELS, 0, arrayLayers);
    (void)mipLevels; // already covered by VK_REMAINING_MIP_LEVELS
}

void CommandList::CopyBufferToImage(VkBuffer buffer, VkImage image,
    uint32_t width, uint32_t height, VkImageAspectFlags aspectMask)
{
    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = aspectMask;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(this->buffer, buffer, image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandList::BlitImageMip(VkImage srcImage, VkImage dstImage,
    uint32_t srcWidth, uint32_t srcHeight, uint32_t srcMip,
    uint32_t dstWidth, uint32_t dstHeight, uint32_t dstMip,
    VkFilter filter, VkImageAspectFlags aspectMask)
{
    VkImageBlit blit {};
    blit.srcSubresource.aspectMask = aspectMask;
    blit.srcSubresource.mipLevel = srcMip;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { (int32_t)srcWidth, (int32_t)srcHeight, 1 };

    blit.dstSubresource.aspectMask = aspectMask;
    blit.dstSubresource.mipLevel = dstMip;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { (int32_t)dstWidth, (int32_t)dstHeight, 1 };

    vkCmdBlitImage(buffer, srcImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);
}

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

CustomError SubmitOneTimeCommands(Device& device,
    const std::function<CustomError(CommandList&)>& recorder)
{
    auto [pool, poolErr] = CreateCommandPool(device);
    if (poolErr)
        return poolErr;

    auto [cmd, allocErr] = AllocateCommandList(device, pool);
    if (allocErr) {
        vkDestroyCommandPool(device.LogicalDevice, pool, nullptr);
        return allocErr;
    }

    if (auto recErr = recorder(cmd)) {
        vkDestroyCommandPool(device.LogicalDevice, pool, nullptr);
        return recErr;
    }

    VkSubmitInfo submit {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd.buffer;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    if (vkCreateFence(device.LogicalDevice, &fenceInfo, nullptr, &fence)
        != VK_SUCCESS) {
        vkDestroyCommandPool(device.LogicalDevice, pool, nullptr);
        return CustomError(ErrorCode::CommandBufferAllocationFailed,
            "Failed to create one-time-submit fence");
    }

    if (vkQueueSubmit(device.GraphicsQueue, 1, &submit, fence) != VK_SUCCESS) {
        vkDestroyFence(device.LogicalDevice, fence, nullptr);
        vkDestroyCommandPool(device.LogicalDevice, pool, nullptr);
        return CustomError(ErrorCode::SwapchainPresentFailed,
            "Failed to submit one-time commands");
    }

    vkWaitForFences(device.LogicalDevice, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device.LogicalDevice, fence, nullptr);
    vkDestroyCommandPool(device.LogicalDevice, pool, nullptr);
    return CustomError();
}

} // namespace yst::core

#include "swapchain/swapchain.hpp"

#include "device/device.hpp"
#include "errors.hpp"

namespace yst::core {

CustomError InitFrames(Device* device, std::vector<FrameData>& frames,
                       uint32_t count) {
    frames.resize(count);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < count; i++) {
        auto [pool, poolErr] = CreateCommandPool(*device);
        if (poolErr) return poolErr;
        frames[i].CommandPool = pool;

        auto [cmd, cmdErr] = AllocateCommandList(*device, pool);
        if (cmdErr) return cmdErr;
        frames[i].CmdList = cmd;

        if (vkCreateSemaphore(device->LogicalDevice, &semaphoreInfo, nullptr,
                              &frames[i].ImageAvailableSemaphore) !=
                VK_SUCCESS ||
            vkCreateFence(device->LogicalDevice, &fenceInfo, nullptr,
                          &frames[i].RenderInFlightFence) != VK_SUCCESS) {
            return CustomError(32, "Failed to create sync objects");
        }
    }
    return CustomError();
}

std::pair<Swapchain, CustomError> CreateSwapchain(
    Device& device, const yst::ywin::Window& window, SwapchainConfig config) {
    Swapchain out;
    out.device = &device;
    out.config = config;

    out.surface = window.GetSurface(device.Instance);
    if (!out.surface)
        return {std::move(out), CustomError(20, "Failed to create Surface")};

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device.PhysicalDevice,
                                         device.GraphicsQueueFamily,
                                         out.surface, &presentSupport);
    if (!presentSupport)
        return {std::move(out),
                CustomError(29,
                            "Selected GPU queue does not support presenting to "
                            "this window!")};

    vkb::SwapchainBuilder builder{device.PhysicalDevice, device.LogicalDevice,
                                  out.surface, device.GraphicsQueueFamily,
                                  device.GraphicsQueueFamily};

    auto swap_ret = builder.set_desired_present_mode(config.PresentMode)
                        .set_desired_format({config.PreferredFormat,
                                             config.PreferredColorSpace})
                        .build();

    if (!swap_ret)
        return {std::move(out), CustomError(21, "Failed to build Swapchain")};

    out.vkbSwapchain = swap_ret.value();
    out.images = out.vkbSwapchain.get_images().value();
    out.imageViews = out.vkbSwapchain.get_image_views().value();

    out.imagePresentationSemaphores.resize(out.images.size());
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (size_t i = 0; i < out.images.size(); i++) {
        vkCreateSemaphore(device.LogicalDevice, &semInfo, nullptr,
                          &out.imagePresentationSemaphores[i]);
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = out.vkbSwapchain.image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.LogicalDevice, &renderPassInfo, nullptr,
                           &out.renderPass) != VK_SUCCESS) {
        return {std::move(out),
                CustomError(27, "Failed to create render pass")};
    }

    if (auto err = out.CreateFramebuffers()) return {std::move(out), err};
    if (auto err = InitFrames(&device, out.frames, config.MaxFramesInFlight))
        return {std::move(out), err};

    return {std::move(out), CustomError()};
}

std::pair<CommandList, CustomError> Swapchain::AcquireNextFrame(
    const yst::ywin::Window& window) {
    FrameData& currentFrame = frames[currentFrameIndex];

    vkWaitForFences(device->LogicalDevice, 1, &currentFrame.RenderInFlightFence,
                    VK_TRUE, UINT64_MAX);

    VkResult result =
        vkAcquireNextImageKHR(device->LogicalDevice, vkbSwapchain.swapchain,
                              UINT64_MAX, currentFrame.ImageAvailableSemaphore,
                              VK_NULL_HANDLE, &currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return {CommandList{},
                CustomError(VK_ERROR_OUT_OF_DATE_KHR, "Out of date")};
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return {CommandList{},
                CustomError(22, "Failed to acquire swapchain image")};
    }

    vkResetFences(device->LogicalDevice, 1, &currentFrame.RenderInFlightFence);

    currentFrame.CmdList.Reset();
    currentFrame.CmdList.Begin();
    currentFrame.CmdList.imageIndex = currentImageIndex;

    return {currentFrame.CmdList, CustomError()};
}

CustomError Swapchain::Present(CommandList& cmd) {
    FrameData& currentFrame = frames[currentFrameIndex];

    cmd.End();

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {currentFrame.ImageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd.buffer;

    VkSemaphore signalSemaphores[] = {
        imagePresentationSemaphores[currentImageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device->GraphicsQueue, 1, &submitInfo,
                      currentFrame.RenderInFlightFence) != VK_SUCCESS) {
        return CustomError(25, "Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {vkbSwapchain.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &currentImageIndex;

    VkResult result = vkQueuePresentKHR(device->GraphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    } else if (result != VK_SUCCESS)
        return CustomError(26, "Failed to present swapchain image");

    currentFrameIndex = (currentFrameIndex + 1) % frames.size();
    return CustomError();
}

CustomError Swapchain::CreateFramebuffers() {
    framebuffers.resize(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); i++) {
        VkImageView attachments[] = {imageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vkbSwapchain.extent.width;
        framebufferInfo.height = vkbSwapchain.extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device->LogicalDevice, &framebufferInfo,
                                nullptr, &framebuffers[i]) != VK_SUCCESS) {
            return CustomError(28, "Failed to create framebuffer");
        }
    }
    return CustomError();
}

void Swapchain::Cleanup() {
    if (!device || device->LogicalDevice == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device->LogicalDevice);

    for (auto& frame : frames) {
        if (frame.RenderInFlightFence)
            vkDestroyFence(device->LogicalDevice, frame.RenderInFlightFence,
                           nullptr);
        if (frame.ImageAvailableSemaphore)
            vkDestroySemaphore(device->LogicalDevice,
                               frame.ImageAvailableSemaphore, nullptr);
        if (frame.CommandPool)
            vkDestroyCommandPool(device->LogicalDevice, frame.CommandPool,
                                 nullptr);
    }
    frames.clear();

    for (auto sem : imagePresentationSemaphores)
        vkDestroySemaphore(device->LogicalDevice, sem, nullptr);
    imagePresentationSemaphores.clear();

    for (auto fb : framebuffers)
        vkDestroyFramebuffer(device->LogicalDevice, fb, nullptr);
    framebuffers.clear();

    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device->LogicalDevice, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    for (auto imageView : imageViews)
        vkDestroyImageView(device->LogicalDevice, imageView, nullptr);
    imageViews.clear();

    if (vkbSwapchain.swapchain != VK_NULL_HANDLE) {
        vkb::destroy_swapchain(vkbSwapchain);
        vkbSwapchain.swapchain = VK_NULL_HANDLE;
    }

    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(device->Instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    device = nullptr;
}

Swapchain::~Swapchain() { Cleanup(); }

Swapchain::Swapchain(Swapchain&& other) noexcept {
    device = other.device;
    config = other.config;
    surface = other.surface;
    vkbSwapchain = other.vkbSwapchain;
    images = std::move(other.images);
    imageViews = std::move(other.imageViews);
    frames = std::move(other.frames);
    currentFrameIndex = other.currentFrameIndex;
    currentImageIndex = other.currentImageIndex;
    imagePresentationSemaphores = std::move(other.imagePresentationSemaphores);
    renderPass = other.renderPass;
    framebuffers = std::move(other.framebuffers);

    other.device = nullptr;
    other.surface = VK_NULL_HANDLE;
    other.vkbSwapchain.swapchain = VK_NULL_HANDLE;
    other.frames.clear();
    other.imagePresentationSemaphores.clear();
    other.renderPass = VK_NULL_HANDLE;
    other.framebuffers.clear();
}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept {
    if (this != &other) {
        Cleanup();
        device = other.device;
        config = other.config;
        surface = other.surface;
        vkbSwapchain = other.vkbSwapchain;
        images = std::move(other.images);
        imageViews = std::move(other.imageViews);
        frames = std::move(other.frames);
        currentFrameIndex = other.currentFrameIndex;
        currentImageIndex = other.currentImageIndex;
        imagePresentationSemaphores =
            std::move(other.imagePresentationSemaphores);
        renderPass = other.renderPass;
        framebuffers = std::move(other.framebuffers);

        other.device = nullptr;
        other.surface = VK_NULL_HANDLE;
        other.vkbSwapchain.swapchain = VK_NULL_HANDLE;
        other.frames.clear();
        other.imagePresentationSemaphores.clear();
        other.renderPass = VK_NULL_HANDLE;
        other.framebuffers.clear();
    }
    return *this;
}

VkRenderPass Swapchain::GetRenderPass() const { return renderPass; }
VkFramebuffer Swapchain::GetCurrentFramebuffer() const {
    return framebuffers[currentImageIndex];
}
VkExtent2D Swapchain::GetExtent() const { return vkbSwapchain.extent; }

CustomError Swapchain::Recreate(const yst::ywin::Window& window) {
    if (!device) return CustomError(40, "Invalid device in swapchain");

    while (window.IsMinimized()) window.WaitEvents();

    vkDeviceWaitIdle(device->LogicalDevice);

    for (auto fb : framebuffers)
        vkDestroyFramebuffer(device->LogicalDevice, fb, nullptr);
    framebuffers.clear();

    vkb::SwapchainBuilder builder{device->PhysicalDevice, device->LogicalDevice,
                                  surface, device->GraphicsQueueFamily,
                                  device->GraphicsQueueFamily};

    auto swap_ret = builder.set_desired_present_mode(config.PresentMode)
                        .set_desired_format({config.PreferredFormat,
                                             config.PreferredColorSpace})
                        .set_old_swapchain(vkbSwapchain)
                        .build();

    if (!swap_ret) return CustomError(41, "Failed to rebuild swapchain");

    for (auto imageView : imageViews)
        vkDestroyImageView(device->LogicalDevice, imageView, nullptr);
    imageViews.clear();

    for (auto sem : imagePresentationSemaphores)
        vkDestroySemaphore(device->LogicalDevice, sem, nullptr);
    imagePresentationSemaphores.clear();

    vkb::destroy_swapchain(vkbSwapchain);

    vkbSwapchain = swap_ret.value();
    images = vkbSwapchain.get_images().value();
    imageViews = vkbSwapchain.get_image_views().value();

    imagePresentationSemaphores.resize(images.size());
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (size_t i = 0; i < images.size(); i++) {
        vkCreateSemaphore(device->LogicalDevice, &semInfo, nullptr,
                          &imagePresentationSemaphores[i]);
    }

    if (auto err = CreateFramebuffers()) return err;
    return CustomError();
}

}  // namespace yst::core

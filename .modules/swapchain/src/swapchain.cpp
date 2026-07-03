#include "swapchain/swapchain.hpp"
#include <volk.h>

#include <GLFW/glfw3.h>

#include <array>

#include "device/device.hpp"
#include "errors.hpp"

namespace yst::core {

SwapchainConfig CreateConfig(SwapchainPreset preset)
{
    SwapchainConfig cfg;
    switch (preset) {
    case SwapchainPreset::Mailbox:
        cfg.PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        cfg.FallbackPresentModes = { VK_PRESENT_MODE_FIFO_KHR };
        return cfg;
    case SwapchainPreset::Immediate:
        cfg.PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        cfg.FallbackPresentModes = { VK_PRESENT_MODE_FIFO_KHR };
        return cfg;
    case SwapchainPreset::WithDepth:
        cfg.Depth.Format = VK_FORMAT_D32_SFLOAT;
        return cfg;
    case SwapchainPreset::Default:
    default:
        return cfg;
    }
}

namespace {

    /// Block until the window has a non-zero framebuffer size. Uses
    /// glfwWaitEvents (via window.WaitEvents) instead of the historical
    /// busy-wait loop, so the CPU doesn't spin while the window is minimised.
    void WaitForNonZeroExtent(const yst::ywin::Window& window, int& width, int& height)
    {
        auto ext = window.GetFramebufferSize();
        width = ext.Width;
        height = ext.Height;
        while (width == 0 || height == 0) {
            window.WaitEvents();
            ext = window.GetFramebufferSize();
            width = ext.Width;
            height = ext.Height;
        }
    }

    /// Re-allocate the per-image presentation semaphores. Each swapchain image
    /// needs its own semaphore so the GPU can signal "this image is ready to
    /// be presented" without colliding with another in-flight image.
    CustomError CreateImagePresentationSemaphores(Device& device,
        std::vector<VkSemaphore>& semaphores, size_t count)
    {
        semaphores.assign(count, VK_NULL_HANDLE);
        VkSemaphoreCreateInfo semInfo { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        for (size_t i = 0; i < count; i++) {
            if (vkCreateSemaphore(device.LogicalDevice, &semInfo, nullptr,
                    &semaphores[i])
                != VK_SUCCESS) {
                // Clean up any we already created so the caller sees an empty
                // vector on failure.
                for (size_t j = 0; j < i; ++j) {
                    vkDestroySemaphore(device.LogicalDevice, semaphores[j], nullptr);
                    semaphores[j] = VK_NULL_HANDLE;
                }
                semaphores.clear();
                return CustomError(ErrorCode::SyncObjectCreationFailed,
                    "Failed to create image presentation semaphore");
            }
        }
        return CustomError();
    }

} // namespace

CustomError InitFrames(Device* device, std::vector<FrameData>& frames,
    uint32_t count)
{
    frames.resize(count);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < count; i++) {
        auto [pool, poolErr] = CreateCommandPool(*device);
        if (poolErr)
            return poolErr;
        frames[i].CommandPool = pool;

        auto [cmd, cmdErr] = AllocateCommandList(*device, pool);
        if (cmdErr)
            return cmdErr;
        frames[i].CmdList = cmd;

        if (vkCreateSemaphore(device->LogicalDevice, &semaphoreInfo, nullptr,
                &frames[i].ImageAvailableSemaphore)
                != VK_SUCCESS
            || vkCreateFence(device->LogicalDevice, &fenceInfo, nullptr,
                   &frames[i].RenderInFlightFence)
                != VK_SUCCESS) {
            return CustomError(ErrorCode::SyncObjectCreationFailed,
                "Failed to create sync objects");
        }
    }
    return CustomError();
}

CustomError Swapchain::CreateRenderPass()
{
    const bool hasDepth = config.Depth.Format != VK_FORMAT_UNDEFINED;

    std::vector<VkAttachmentDescription> attachments;
    attachments.reserve(hasDepth ? 2 : 1);

    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = vkbSwapchain.image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = config.Color.LoadOp;
    colorAttachment.storeOp = config.Color.StoreOp;
    colorAttachment.stencilLoadOp = config.Color.StencilLoadOp;
    colorAttachment.stencilStoreOp = config.Color.StencilStoreOp;
    colorAttachment.initialLayout = config.Color.InitialLayout;
    colorAttachment.finalLayout = config.Color.FinalLayout;
    attachments.push_back(colorAttachment);

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    VkAttachmentDescription depthAttachment {};
    if (hasDepth) {
        depthAttachment.format = config.Depth.Format;
        depthAttachment.samples = config.Depth.Samples;
        depthAttachment.loadOp = config.Depth.LoadOp;
        depthAttachment.storeOp = config.Depth.StoreOp;
        depthAttachment.stencilLoadOp = config.Depth.StencilLoadOp;
        depthAttachment.stencilStoreOp = config.Depth.StencilStoreOp;
        depthAttachment.initialLayout = config.Depth.InitialLayout;
        depthAttachment.finalLayout = config.Depth.FinalLayout;
        attachments.push_back(depthAttachment);

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : nullptr;

    VkSubpassDependency dependency {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = config.ExternalDependency.SrcStageMask;
    dependency.srcAccessMask = config.ExternalDependency.SrcAccessMask;
    dependency.dstStageMask = config.ExternalDependency.DstStageMask;
    dependency.dstAccessMask = config.ExternalDependency.DstAccessMask;

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device->LogicalDevice, &renderPassInfo, nullptr,
            &renderPass)
        != VK_SUCCESS) {
        return CustomError(ErrorCode::RenderPassCreationFailed,
            "Failed to create render pass");
    }
    return CustomError();
}

CustomError Swapchain::BuildSwapchain(int width, int height,
    const yst::ywin::Window& window, bool useOldSwapchain)
{
    vkb::SwapchainBuilder builder { device->PhysicalDevice,
        device->LogicalDevice, surface, device->GraphicsQueueFamily,
        device->GraphicsQueueFamily };

    auto b = builder.set_desired_present_mode(config.PresentMode)
                 .set_desired_format({ config.PreferredFormat,
                     config.PreferredColorSpace })
                 .set_desired_extent(width, height);

    // Tell vkb to also accept these present modes so it can fall back
    // gracefully if the desired one is unsupported.
    for (VkPresentModeKHR m : config.FallbackPresentModes) {
        b = b.add_fallback_present_mode(m);
    }

    if (useOldSwapchain) {
        b = b.set_old_swapchain(vkbSwapchain);
    }

    auto swap_ret = b.build();
    if (!swap_ret)
        return CustomError(ErrorCode::SwapchainBuildFailed,
            "Failed to build swapchain: " + swap_ret.error().message());

    // When recreating, the old vkb::Swapchain must be explicitly destroyed
    // BEFORE we overwrite the field — otherwise the underlying VkSwapchainKHR
    // handle leaks (vkb::Swapchain is a value type, not a unique_ptr).
    if (useOldSwapchain && vkbSwapchain.swapchain != VK_NULL_HANDLE) {
        vkb::destroy_swapchain(vkbSwapchain);
    }

    vkbSwapchain = swap_ret.value();
    images = vkbSwapchain.get_images().value();
    imageViews = vkbSwapchain.get_image_views().value();

    return CustomError();
}

std::pair<Swapchain, CustomError> CreateSwapchain(
    Device& device, const yst::ywin::Window& window, SwapchainConfig config)
{
    Swapchain out;
    out.device = &device;
    out.config = config;

    out.surface = window.GetSurface(device.Instance);
    if (!out.surface)
        return { std::move(out),
            CustomError(ErrorCode::SurfaceCreationFailed,
                "Failed to create Surface") };

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device.PhysicalDevice,
        device.GraphicsQueueFamily, out.surface, &presentSupport);
    if (!presentSupport)
        return { std::move(out),
            CustomError(ErrorCode::PresentNotSupported,
                "Selected GPU queue does not support presenting to this window!") };

    int width = 0, height = 0;
    WaitForNonZeroExtent(window, width, height);

    if (auto err = out.BuildSwapchain(width, height, window, /*useOld=*/false))
        return { std::move(out), err };

    if (auto err = CreateImagePresentationSemaphores(device,
            out.imagePresentationSemaphores, out.images.size()))
        return { std::move(out), err };

    if (auto err = out.CreateRenderPass())
        return { std::move(out), err };

    if (auto err = out.CreateFramebuffers())
        return { std::move(out), err };
    if (auto err = InitFrames(&device, out.frames, config.MaxFramesInFlight))
        return { std::move(out), err };

    return { std::move(out), CustomError() };
}

std::pair<CommandList, CustomError> Swapchain::AcquireNextFrame(
    const yst::ywin::Window& window)
{
    FrameData& currentFrame = frames[currentFrameIndex];

    vkWaitForFences(device->LogicalDevice, 1, &currentFrame.RenderInFlightFence,
        VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(device->LogicalDevice,
        vkbSwapchain.swapchain, UINT64_MAX,
        currentFrame.ImageAvailableSemaphore,
        VK_NULL_HANDLE, &currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        needsRecreate_ = true;
        return { CommandList {},
            CustomError(ErrorCode::SwapchainOutOfDate, "Out of date") };
    }
    if (result == VK_SUBOPTIMAL_KHR) {
        // Image was acquired successfully, but the swapchain is suboptimal.
        // Continue with this frame and signal that the next acquire needs
        // a recreate.
        needsRecreate_ = true;
    } else if (result != VK_SUCCESS) {
        return { CommandList {},
            CustomError(ErrorCode::SwapchainImageAcquireFailed,
                "Failed to acquire swapchain image") };
    }

    vkResetFences(device->LogicalDevice, 1, &currentFrame.RenderInFlightFence);

    currentFrame.CmdList.Reset();
    currentFrame.CmdList.Begin();
    currentFrame.CmdList.imageIndex = currentImageIndex;

    return { currentFrame.CmdList, CustomError() };
}

CustomError Swapchain::Present(CommandList& cmd)
{
    FrameData& currentFrame = frames[currentFrameIndex];

    cmd.End();

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { currentFrame.ImageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd.buffer;

    VkSemaphore signalSemaphores[] = {
        imagePresentationSemaphores[currentImageIndex]
    };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device->GraphicsQueue, 1, &submitInfo,
            currentFrame.RenderInFlightFence)
        != VK_SUCCESS) {
        return CustomError(ErrorCode::SwapchainPresentFailed,
            "Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { vkbSwapchain.swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &currentImageIndex;

    VkResult result = vkQueuePresentKHR(device->GraphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Don't return an error — the frame was already submitted and the
        // fence will be signaled. Just flag that the caller should recreate
        // before the next acquire.
        needsRecreate_ = true;
    } else if (result != VK_SUCCESS) {
        return CustomError(ErrorCode::SwapchainPresentFailed,
            "Failed to present swapchain image");
    }

    currentFrameIndex = (currentFrameIndex + 1) % frames.size();
    return CustomError();
}

CustomError Swapchain::CreateFramebuffers()
{
    const bool hasDepth = config.Depth.Format != VK_FORMAT_UNDEFINED;

    if (hasDepth && config.DepthImageViewOverride == VK_NULL_HANDLE) {
        return CustomError(ErrorCode::FramebufferCreationFailed,
            "Depth attachment requested but no DepthImageViewOverride was provided");
    }

    framebuffers.resize(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); i++) {
        std::array<VkImageView, 2> attachments;
        attachments[0] = imageViews[i];
        uint32_t attachmentCount = 1;
        if (hasDepth) {
            attachments[1] = config.DepthImageViewOverride;
            attachmentCount = 2;
        }

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = attachmentCount;
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = vkbSwapchain.extent.width;
        framebufferInfo.height = vkbSwapchain.extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device->LogicalDevice, &framebufferInfo,
                nullptr, &framebuffers[i])
            != VK_SUCCESS) {
            return CustomError(ErrorCode::FramebufferCreationFailed,
                "Failed to create framebuffer");
        }
    }
    return CustomError();
}

void Swapchain::Cleanup()
{
    if (!device || device->LogicalDevice == VK_NULL_HANDLE)
        return;

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

    needsRecreate_ = false;
    device = nullptr;
}

Swapchain::~Swapchain() { Cleanup(); }

Swapchain::Swapchain(Swapchain&& other) noexcept
{
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
    needsRecreate_ = other.needsRecreate_;

    other.device = nullptr;
    other.surface = VK_NULL_HANDLE;
    other.vkbSwapchain.swapchain = VK_NULL_HANDLE;
    other.frames.clear();
    other.imagePresentationSemaphores.clear();
    other.renderPass = VK_NULL_HANDLE;
    other.framebuffers.clear();
    other.needsRecreate_ = false;
}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
{
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
        imagePresentationSemaphores = std::move(other.imagePresentationSemaphores);
        renderPass = other.renderPass;
        framebuffers = std::move(other.framebuffers);
        needsRecreate_ = other.needsRecreate_;

        other.device = nullptr;
        other.surface = VK_NULL_HANDLE;
        other.vkbSwapchain.swapchain = VK_NULL_HANDLE;
        other.frames.clear();
        other.imagePresentationSemaphores.clear();
        other.renderPass = VK_NULL_HANDLE;
        other.framebuffers.clear();
        other.needsRecreate_ = false;
    }
    return *this;
}

VkRenderPass Swapchain::GetRenderPass() const { return renderPass; }
VkFramebuffer Swapchain::GetCurrentFramebuffer() const
{
    return framebuffers[currentImageIndex];
}
VkExtent2D Swapchain::GetExtent() const { return vkbSwapchain.extent; }

CustomError Swapchain::Recreate(const yst::ywin::Window& window)
{
    if (!device)
        return CustomError(ErrorCode::SwapchainRebuildFailed,
            "Invalid device in swapchain");

    int width = 0, height = 0;
    WaitForNonZeroExtent(window, width, height);

    vkDeviceWaitIdle(device->LogicalDevice);

    // Tear down everything tied to the OLD swapchain BEFORE BuildSwapchain
    // overwrites the imageViews vector (the OLD image-view handles are a
    // separate allocation from the swapchain itself and would leak if we
    // lost the references to them).
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

    for (auto sem : imagePresentationSemaphores)
        vkDestroySemaphore(device->LogicalDevice, sem, nullptr);
    imagePresentationSemaphores.clear();

    // BuildSwapchain(useOld=true) will:
    //   - use set_old_swapchain(current vkbSwapchain) for the new build
    //   - destroy the OLD VkSwapchainKHR via vkb::destroy_swapchain
    //   - replace vkbSwapchain with the new one
    //   - refresh `images` and `imageViews` from the new swapchain
    if (auto err = BuildSwapchain(width, height, window, /*useOld=*/true))
        return err;

    if (auto err = CreateImagePresentationSemaphores(*device,
            imagePresentationSemaphores, images.size()))
        return err;

    if (auto err = CreateRenderPass())
        return err;

    if (auto err = CreateFramebuffers())
        return err;

    needsRecreate_ = false;
    return CustomError();
}

} // namespace yst::core

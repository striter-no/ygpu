#pragma once
#include <command/command.hpp>
#include <device/device.hpp>
#include <vector>
#include <window/window.hpp>

#include "config.hpp"

namespace yst::core {

struct FrameData {
    VkCommandPool CommandPool = VK_NULL_HANDLE;
    CommandList CmdList;

    // Sync
    VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;
    VkFence RenderInFlightFence = VK_NULL_HANDLE;
};

class Swapchain {
private:
    Device* device = nullptr;
    SwapchainConfig config;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    vkb::Swapchain vkbSwapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

    std::vector<FrameData> frames;
    uint32_t currentFrameIndex = 0;
    uint32_t currentImageIndex = 0;

    std::vector<VkSemaphore> imagePresentationSemaphores;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;

    /// Set when AcquireNextFrame / Present encounters VK_ERROR_OUT_OF_DATE
    /// or VK_SUBOPTIMAL_KHR. Caller is expected to check NeedsRecreate() and
    /// call Recreate(). Replaces the historical empty-branch-on-suboptimal
    /// "for simplicity" stub.
    bool needsRecreate_ = false;

    void Cleanup();
    CustomError CreateFramebuffers();
    CustomError CreateRenderPass();
    CustomError BuildSwapchain(int width, int height,
        const yst::ywin::Window& window, bool useOldSwapchain);

public:
    Swapchain() = default;
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&& other) noexcept;
    Swapchain& operator=(Swapchain&& other) noexcept;

    std::pair<CommandList, CustomError> AcquireNextFrame(
        const yst::ywin::Window& window);
    CustomError Present(CommandList& cmd);
    CustomError Recreate(const yst::ywin::Window& window);

    /// True if the most recent Acquire/Present returned OUT_OF_DATE or
    /// SUBOPTIMAL and the swapchain should be recreated before the next
    /// frame. Cleared by a successful Recreate().
    bool NeedsRecreate() const noexcept { return needsRecreate_; }

    friend std::pair<Swapchain, CustomError> CreateSwapchain(
        Device& device, const yst::ywin::Window& window,
        SwapchainConfig config);

    VkRenderPass GetRenderPass() const;
    VkFramebuffer GetCurrentFramebuffer() const;
    VkExtent2D GetExtent() const;
    const SwapchainConfig& GetConfig() const { return config; }
};

std::pair<Swapchain, CustomError> CreateSwapchain(
    Device& device, const yst::ywin::Window& window,
    SwapchainConfig config);

inline std::pair<Swapchain, CustomError> CreateSwapchain(
    Device& device, const yst::ywin::Window& window)
{
    return CreateSwapchain(device, window, CreateConfig(SwapchainPreset::Default));
}

} // namespace yst::core


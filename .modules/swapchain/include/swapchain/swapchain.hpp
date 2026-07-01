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

    void Cleanup();
    CustomError CreateFramebuffers();

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

    friend std::pair<Swapchain, CustomError> CreateSwapchain(
        Device& device, const yst::ywin::Window& window,
        SwapchainConfig config);

    VkRenderPass GetRenderPass() const;
    VkFramebuffer GetCurrentFramebuffer() const;
    VkExtent2D GetExtent() const;
    const SwapchainConfig& GetConfig() const { return config; }
};

// Заметь: config передается с дефолтным значением!
std::pair<Swapchain, CustomError> CreateSwapchain(
    Device& device, const yst::ywin::Window& window,
    SwapchainConfig config = SwapchainConfig{});

}  // namespace yst::core

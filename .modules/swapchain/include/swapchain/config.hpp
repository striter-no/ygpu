#pragma once
#include <vulkan/vulkan.h>

namespace yst::core {

struct SwapchainConfig {
    uint32_t MaxFramesInFlight = 2;

    VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;

    VkFormat PreferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR PreferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    float ClearColor[4] = { 0.05f, 0.05f, 0.2f, 1.0f };
};

} // namespace yst::core

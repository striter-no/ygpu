#include "device/config.hpp"

namespace yst::core {

DeviceConfig CreateConfig(yst::gpuc::Preset preset)
{
    DeviceConfig cfg;

    // Common default: swapchain extension required.
    cfg.DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    switch (preset) {
    case yst::gpuc::DEFAULT_CONFIG:
        cfg.PreferIntegratedGPU = false;
        cfg.EnableDebug = false;
        cfg.MinVulkanVersion = { 1, 2, 0 };
        return cfg;

    case yst::gpuc::DEBUG_CONFIG:
        cfg.PreferIntegratedGPU = false;
        cfg.EnableDebug = true;
        cfg.MinVulkanVersion = { 1, 2, 0 };
        return cfg;

    case yst::gpuc::HEADLESS_CONFIG:
        cfg.PreferIntegratedGPU = true;
        cfg.EnableDebug = false;
        cfg.MinVulkanVersion = { 1, 1, 0 };
        cfg.RequirePresent = false;
        // Headless rendering has no surface — swapchain extension isn't
        // strictly required, but keeping it allows the same device to be
        // paired with a surface later without re-selection.
        return cfg;

    case yst::gpuc::HIGH_PERFORMANCE:
        cfg.PreferIntegratedGPU = false;
        cfg.EnableDebug = true;
        cfg.MinVulkanVersion = { 1, 3, 0 };
        cfg.RequirePresent = true;
        return cfg;

    case yst::gpuc::LOW_POWER:
        cfg.PreferIntegratedGPU = true;
        cfg.EnableDebug = false;
        cfg.MinVulkanVersion = { 1, 1, 0 };
        cfg.RequirePresent = true;
        return cfg;
    }

    return cfg;
}

} // namespace yst::core


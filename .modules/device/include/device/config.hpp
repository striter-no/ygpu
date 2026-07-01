#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string>
#include <vector>

namespace yst::gpuc {

/// High-level rendering backend selector. Currently only VULKAN is
/// implemented; MOCK is reserved for a future CPU/software backend usable
/// from unit tests that don't need a real GPU.
enum Backend { BACKEND_VULKAN,
    BACKEND_MOCK };

/// Named presets for typical DeviceConfig profiles.
enum Preset {
    DEFAULT_CONFIG = 0, ///< Discrete GPU if available, debug enabled.
    HEADLESS_CONFIG, ///< Integrated if available, debug off.
    HIGH_PERFORMANCE, ///< Force discrete, require Vulkan 1.3, portability off.
    LOW_POWER, ///< Prefer integrated, allow 1.1 minimum.
};

/// Pack/unpack a Vulkan version number (VK_API_VERSION_X_Y).
struct ApiVersion {
    uint32_t Major = 1;
    uint32_t Minor = 2;
    uint32_t Patch = 0;

    uint32_t Pack() const noexcept
    {
        return VK_MAKE_VERSION(Major, Minor, Patch);
    }

    static ApiVersion Unpack(uint32_t packed) noexcept
    {
        return {
            VK_VERSION_MAJOR(packed),
            VK_VERSION_MINOR(packed),
            VK_VERSION_PATCH(packed),
        };
    }
};

} // namespace yst::gpuc

namespace yst::core {

/// Layer 2 configuration for a logical device + its physical GPU selection.
///
/// Defaults match the historical hardcoded behaviour of `CreateDevice`, so
/// `DeviceConfig{}` is equivalent to the old "always Vulkan 1.2, swapchain
/// extension, discrete preferred, debug on" code path.
struct DeviceConfig {
    /// Render backend. Must be BACKEND_VULKAN today; BACKEND_MOCK returns
    /// ErrorCode::UnsupportedBackend from CreateDevice.
    yst::gpuc::Backend PreferredBackend = yst::gpuc::BACKEND_VULKAN;

    /// Instance / app identification passed to the Vulkan driver.
    std::string AppName = "YST Engine";
    std::string EngineName = "YST Engine";
    yst::gpuc::ApiVersion AppVersion { 0, 1, 0 };
    yst::gpuc::ApiVersion EngineVersion { 0, 1, 0 };

    /// Minimum Vulkan API version required on the physical device.
    /// Used for both vkb::PhysicalDeviceSelector::set_minimum_version and
    /// VmaAllocatorCreateInfo::vulkanApiVersion (so VMA and VkBootstrap stay
    /// consistent — they used to disagree: 1.2 minimum vs 1.0 allocator).
    yst::gpuc::ApiVersion MinVulkanVersion { 1, 2, 0 };

    /// Instance-level extensions to enable (e.g. VK_EXT_debug_utils).
    std::vector<const char*> InstanceExtensions;

    /// Device-level extensions required for physical device selection.
    /// Defaults to { VK_KHR_SWAPCHAIN_EXTENSION_NAME }.
    std::vector<const char*> DeviceExtensions;

    /// Prefer an integrated GPU over discrete. Off by default.
    bool PreferIntegratedGPU = false;

    /// Require the graphics queue family to support presentation.
    /// Off by default to allow headless selection; the swapchain module
    /// re-checks present support against the chosen surface.
    bool RequirePresent = false;

    /// Enable validation layers + debug messenger. On by default in builds
    /// that opt into debug; release builds should flip this off.
    bool EnableDebug = true;
};

/// Resolve a named preset into a fully-populated DeviceConfig.
DeviceConfig CreateConfig(yst::gpuc::Preset preset);

} // namespace yst::core

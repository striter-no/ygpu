#pragma once
#include <volk.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <vulkan/vulkan.h>

#include <errors.hpp>
#include <string>
#include <vector>

#include "config.hpp"

namespace yst::core {

class Device {
public:
    std::size_t GetCompiledDeviceSize() noexcept
    {
        return sizeof(Device);
    }

    PFN_vkGetInstanceProcAddr
        vkGetInstanceProcAddr
        = nullptr;

    VkInstance Instance = VK_NULL_HANDLE;
    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    VkDevice LogicalDevice = VK_NULL_HANDLE;
    VkQueue GraphicsQueue = VK_NULL_HANDLE;
    uint32_t GraphicsQueueFamily = 0;
    VkQueue ComputeQueue = VK_NULL_HANDLE;
    uint32_t ComputeQueueFamily = 0;
    VmaAllocator Allocator = VK_NULL_HANDLE;

    vkb::Instance vkbInstance {};
    vkb::Device vkbDevice {};

    Device() = default;
    ~Device();
    void Destroy();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

    /// Human-readable name of the selected physical GPU.
    /// Empty if the device was never successfully created.
    std::string GetDeviceName() const;

    /// The Vulkan API version actually negotiated with the driver
    /// (may be >= DeviceConfig::MinVulkanVersion).
    yst::gpuc::ApiVersion GetActiveApiVersion() const;
};

std::pair<Device, CustomError> CreateDevice(const DeviceConfig& config);

/// Convenience overload (Layer 0): create a device with the DEFAULT_CONFIG
/// preset. Equivalent to `CreateDevice(CreateConfig(gpuc::DEFAULT_CONFIG))`.
inline std::pair<Device, CustomError> CreateDevice()
{
    return CreateDevice(CreateConfig(yst::gpuc::DEFAULT_CONFIG));
}

/// Returns number of physical GPUs in the system.
/// Creates and destroys temporal Vulkan instance.
uint32_t GetAvailableDeviceCount();

} // namespace yst::core

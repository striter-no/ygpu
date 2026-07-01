#pragma once
#include <VkBootstrap.h>
#include <errors.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace yst::core {

struct DeviceConfig {
  bool PreferIntegratedGPU = false;
  bool EnableDebug = true;
  bool HeadlessRun = false;
};

class Device {
public:
  VkInstance Instance = VK_NULL_HANDLE;
  VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
  VkDevice LogicalDevice = VK_NULL_HANDLE;
  VkQueue GraphicsQueue = VK_NULL_HANDLE;
  uint32_t GraphicsQueueFamily = 0;
  VmaAllocator Allocator = VK_NULL_HANDLE;

  vkb::Instance vkbInstance;
  vkb::Device vkbDevice;

  Device() = default;
  ~Device();
  void Destroy();

  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;

  Device(Device &&other) noexcept {
    Instance = other.Instance;
    PhysicalDevice = other.PhysicalDevice;
    LogicalDevice = other.LogicalDevice;
    GraphicsQueue = other.GraphicsQueue;
    GraphicsQueueFamily = other.GraphicsQueueFamily;
    Allocator = other.Allocator;
    vkbInstance = other.vkbInstance;
    vkbDevice = other.vkbDevice;

    other.Instance = VK_NULL_HANDLE;
    other.LogicalDevice = VK_NULL_HANDLE;
    other.Allocator = VK_NULL_HANDLE;
  }

  Device &operator=(Device &&other) noexcept {
    if (this != &other) {
      Destroy();

      Instance = other.Instance;
      PhysicalDevice = other.PhysicalDevice;
      LogicalDevice = other.LogicalDevice;
      GraphicsQueue = other.GraphicsQueue;
      GraphicsQueueFamily = other.GraphicsQueueFamily;
      Allocator = other.Allocator;
      vkbInstance = other.vkbInstance;
      vkbDevice = other.vkbDevice;

      other.Instance = VK_NULL_HANDLE;
      other.LogicalDevice = VK_NULL_HANDLE;
      other.Allocator = VK_NULL_HANDLE;
    }
    return *this;
  }
};

std::pair<Device, CustomError> CreateDevice(const DeviceConfig &config);

} // namespace yst::core

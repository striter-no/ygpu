#include <device.hpp>

namespace yst::core {

std::pair<Device, CustomError> CreateDevice(const DeviceConfig &config) {
  Device outDevice;

  // 1. Vulkan init
  vkb::InstanceBuilder builder;
  auto inst_ret = builder.set_app_name("YST Engine")
                      .request_validation_layers(config.EnableDebug)
                      .use_default_debug_messenger()
                      .build();

  if (!inst_ret) {
    return {std::move(outDevice),
            CustomError(1, "Failed to create Vulkan Instance: " +
                               inst_ret.error().message())};
  }
  outDevice.vkbInstance = inst_ret.value();
  outDevice.Instance = outDevice.vkbInstance.instance;

  // 2. Physical device
  vkb::PhysicalDeviceSelector selector{outDevice.vkbInstance};

  auto gpuPreference = config.PreferIntegratedGPU
                           ? vkb::PreferredDeviceType::integrated
                           : vkb::PreferredDeviceType::discrete;

  auto phys_ret = selector.set_minimum_version(1, 2)
                      .prefer_gpu_device_type(gpuPreference)
                      .require_present(!config.HeadlessRun)
                      .select();

  if (!phys_ret) {
    return {std::move(outDevice),
            CustomError(2, "Failed to select physical device: " +
                               phys_ret.error().message())};
  }
  vkb::PhysicalDevice physicalDevice = phys_ret.value();
  outDevice.PhysicalDevice = physicalDevice.physical_device;

  vkb::DeviceBuilder deviceBuilder{physicalDevice};
  auto dev_ret = deviceBuilder.build();

  if (!dev_ret) {
    return {std::move(outDevice),
            CustomError(3, "Failed to create logical device: " +
                               dev_ret.error().message())};
  }
  outDevice.vkbDevice = dev_ret.value();
  outDevice.LogicalDevice = outDevice.vkbDevice.device;

  auto graphicsQueue_ret =
      outDevice.vkbDevice.get_queue(vkb::QueueType::graphics);
  auto graphicsQueueFam_ret =
      outDevice.vkbDevice.get_queue_index(vkb::QueueType::graphics);

  if (!graphicsQueue_ret || !graphicsQueueFam_ret) {
    return {std::move(outDevice),
            CustomError(4, "Failed to get graphics queue")};
  }
  outDevice.GraphicsQueue = graphicsQueue_ret.value();
  outDevice.GraphicsQueueFamily = graphicsQueueFam_ret.value();

  // 5. VMA init
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = outDevice.PhysicalDevice;
  allocatorInfo.device = outDevice.LogicalDevice;
  allocatorInfo.instance = outDevice.Instance;

  allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;

  if (vmaCreateAllocator(&allocatorInfo, &outDevice.Allocator) != VK_SUCCESS) {
    return {std::move(outDevice),
            CustomError(5, "Failed to create VMA Allocator")};
  }

  return {std::move(outDevice), CustomError()};
}

void Device::Destroy() {
  if (Allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(Allocator);
    Allocator = VK_NULL_HANDLE;
  }

  if (LogicalDevice != VK_NULL_HANDLE) {
    vkb::destroy_device(vkbDevice);
    LogicalDevice = VK_NULL_HANDLE;
  }

  if (Instance != VK_NULL_HANDLE) {
    vkb::destroy_instance(vkbInstance);
    Instance = VK_NULL_HANDLE;
  }
}

Device::~Device() { Destroy(); }
} // namespace yst::core

#include <device/device.hpp>

#include <cstring>

#include "device/config.hpp"

namespace yst::core {

namespace {

    /// Build the vkb::InstanceBuilder using every relevant field of `config`.
    /// Returns the built instance or the vkb error message on failure.
    vkb::Result<vkb::Instance> BuildInstance(const DeviceConfig& config)
    {
        vkb::InstanceBuilder builder;
        auto b = builder.set_app_name(config.AppName.c_str())
                     .set_engine_name(config.EngineName.c_str())
                     .require_api_version(config.MinVulkanVersion.Major,
                         config.MinVulkanVersion.Minor,
                         config.MinVulkanVersion.Patch)
                     .set_app_version(config.AppVersion.Pack())
                     .set_engine_version(config.EngineVersion.Pack())
                     .request_validation_layers(config.EnableDebug);

        if (config.EnableDebug) {
            b = b.use_default_debug_messenger();
        }

        for (const char* ext : config.InstanceExtensions) {
            b = b.enable_extension(ext);
        }

        return b.build();
    }

} // namespace

std::pair<Device, CustomError> CreateDevice(const DeviceConfig& config)
{
    Device outDevice;

    // 1. Reject unsupported backends up-front. The Backend enum exists in
    //    the public API precisely so callers can opt into MOCK when it
    //    becomes available — but for now anything other than Vulkan is a
    //    hard error rather than a silent fallthrough.
    if (config.PreferredBackend != yst::gpuc::BACKEND_VULKAN) {
        return { std::move(outDevice),
            CustomError(ErrorCode::UnsupportedBackend,
                "Only BACKEND_VULKAN is implemented in this build") };
    }

    // 2. Instance.
    auto inst_ret = BuildInstance(config);
    if (!inst_ret) {
        return { std::move(outDevice),
            CustomError(ErrorCode::InstanceCreationFailed,
                "Failed to create Vulkan Instance: "
                    + inst_ret.error().message()) };
    }
    outDevice.vkbInstance = inst_ret.value();
    outDevice.Instance = outDevice.vkbInstance.instance;

    // 3. Physical device selection.
    vkb::PhysicalDeviceSelector selector { outDevice.vkbInstance };

    auto gpuPreference = config.PreferIntegratedGPU
        ? vkb::PreferredDeviceType::integrated
        : vkb::PreferredDeviceType::discrete;

    auto phys_builder = selector.set_minimum_version(
                                    config.MinVulkanVersion.Major,
                                    config.MinVulkanVersion.Minor)
                            .prefer_gpu_device_type(gpuPreference)
                            .require_present(config.RequirePresent);

    // Required device extensions. We must copy into a stable vector because
    // vkb stores pointers, not values.
    std::vector<const char*> requiredExts;
    if (config.DeviceExtensions.empty()) {
        // Backwards-compatible default if caller clears the list.
        requiredExts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    } else {
        requiredExts = config.DeviceExtensions;
    }
    for (const char* ext : requiredExts) {
        phys_builder = phys_builder.add_required_extension(ext);
    }

    auto phys_ret = phys_builder.select();
    if (!phys_ret) {
        return { std::move(outDevice),
            CustomError(ErrorCode::PhysicalDeviceSelectionFailed,
                "Failed to select physical device: "
                    + phys_ret.error().message()) };
    }
    vkb::PhysicalDevice physicalDevice = phys_ret.value();
    outDevice.PhysicalDevice = physicalDevice.physical_device;

    // 4. Logical device.
    vkb::DeviceBuilder deviceBuilder { physicalDevice };
    auto dev_ret = deviceBuilder.build();
    if (!dev_ret) {
        return { std::move(outDevice),
            CustomError(ErrorCode::LogicalDeviceCreationFailed,
                "Failed to create logical device: "
                    + dev_ret.error().message()) };
    }
    outDevice.vkbDevice = dev_ret.value();
    outDevice.LogicalDevice = outDevice.vkbDevice.device;

    // 5. Graphics queue.
    auto graphicsQueue_ret = outDevice.vkbDevice.get_queue(vkb::QueueType::graphics);
    auto graphicsQueueFam_ret = outDevice.vkbDevice.get_queue_index(vkb::QueueType::graphics);
    if (!graphicsQueue_ret || !graphicsQueueFam_ret) {
        return { std::move(outDevice),
            CustomError(ErrorCode::GraphicsQueueNotFound,
                "Failed to get graphics queue") };
    }
    outDevice.GraphicsQueue = graphicsQueue_ret.value();
    outDevice.GraphicsQueueFamily = graphicsQueueFam_ret.value();

    // 6. VMA allocator. Use the same API version we negotiated with
    //    VkBootstrap so VMA's internal function pointer table matches the
    //    device's actual capabilities (this used to be hardcoded to 1.0
    //    while the device required 1.2 — a real bug masked by the fact that
    //    VMA degrades gracefully when an entry point is missing).
    VmaAllocatorCreateInfo allocatorInfo {};
    allocatorInfo.physicalDevice = outDevice.PhysicalDevice;
    allocatorInfo.device = outDevice.LogicalDevice;
    allocatorInfo.instance = outDevice.Instance;
    allocatorInfo.vulkanApiVersion = config.MinVulkanVersion.Pack();

    if (vmaCreateAllocator(&allocatorInfo, &outDevice.Allocator) != VK_SUCCESS) {
        return { std::move(outDevice),
            CustomError(ErrorCode::VmaAllocatorCreationFailed,
                "Failed to create VMA Allocator") };
    }

    return { std::move(outDevice), CustomError() };
}

void Device::Destroy()
{
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

Device::Device(Device&& other) noexcept
{
    Instance = other.Instance;
    PhysicalDevice = other.PhysicalDevice;
    LogicalDevice = other.LogicalDevice;
    GraphicsQueue = other.GraphicsQueue;
    GraphicsQueueFamily = other.GraphicsQueueFamily;
    Allocator = other.Allocator;
    vkbInstance = other.vkbInstance;
    vkbDevice = other.vkbDevice;

    other.Instance = VK_NULL_HANDLE;
    other.PhysicalDevice = VK_NULL_HANDLE;
    other.LogicalDevice = VK_NULL_HANDLE;
    other.GraphicsQueue = VK_NULL_HANDLE;
    other.GraphicsQueueFamily = 0;
    other.Allocator = VK_NULL_HANDLE;
}

Device& Device::operator=(Device&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
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
    other.PhysicalDevice = VK_NULL_HANDLE;
    other.LogicalDevice = VK_NULL_HANDLE;
    other.GraphicsQueue = VK_NULL_HANDLE;
    other.GraphicsQueueFamily = 0;
    other.Allocator = VK_NULL_HANDLE;

    return *this;
}

std::string Device::GetDeviceName() const
{
    if (PhysicalDevice == VK_NULL_HANDLE)
        return {};
    VkPhysicalDeviceProperties props {};
    vkGetPhysicalDeviceProperties(PhysicalDevice, &props);
    return props.deviceName;
}

yst::gpuc::ApiVersion Device::GetActiveApiVersion() const
{
    if (PhysicalDevice == VK_NULL_HANDLE)
        return {};
    VkPhysicalDeviceProperties props {};
    vkGetPhysicalDeviceProperties(PhysicalDevice, &props);
    return yst::gpuc::ApiVersion::Unpack(props.apiVersion);
}

} // namespace yst::core


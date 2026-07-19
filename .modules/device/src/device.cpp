#include <iostream>
#ifdef __APPLE__
#include <dlfcn.h>
#endif

#include <device/device.hpp>

#include <cstring>

#include "device/config.hpp"

namespace yst::core {

namespace {

    /// Build the vkb::InstanceBuilder using every relevant field of `config`.
    /// Returns the built instance or the vkb error message on failure.
    vkb::Result<vkb::Instance> BuildInstance(const DeviceConfig& config)
    {
#ifdef __APPLE__
        bool enableValidation = false;
#else
        bool enableValidation = config.EnableDebug;
#endif
        vkb::InstanceBuilder builder;
        auto b = builder.set_app_name(config.AppName.c_str())
                     .set_engine_name(config.EngineName.c_str())
                     .require_api_version(config.MinVulkanVersion.Major,
                         config.MinVulkanVersion.Minor,
                         config.MinVulkanVersion.Patch)
                     .set_app_version(config.AppVersion.Pack())
                     .set_engine_version(config.EngineVersion.Pack())
                     .request_validation_layers(enableValidation);

        if (enableValidation) {
            b = b.use_default_debug_messenger();
        }

        for (const char* ext : config.InstanceExtensions) {
            std::cerr << "[debug] enabling: " << ext << std::endl;
            b = b.enable_extension(ext);
        }

        // #ifdef __APPLE__
        //         b = b.enable_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        // #endif

        return b.build();
    }

} // namespace

std::pair<Device, CustomError> CreateDevice(const DeviceConfig& config)
{
    Device outDevice;

    if (config.PreferredBackend != yst::gpuc::BACKEND_VULKAN) {
        return { std::move(outDevice),
            CustomError(ErrorCode::UnsupportedBackend,
                "Only BACKEND_VULKAN is implemented in this build") };
    }

#ifdef __APPLE__
    void* moltenVkModule = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!moltenVkModule) {
        return { std::move(outDevice),
            CustomError(ErrorCode::InstanceCreationFailed, "Failed to load libMoltenVK.dylib") };
    }

    auto fp_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(moltenVkModule, "vkGetInstanceProcAddr");
    if (!fp_vkGetInstanceProcAddr) {
        return { std::move(outDevice),
            CustomError(ErrorCode::InstanceCreationFailed, "Failed to find vkGetInstanceProcAddr in MoltenVK") };
    }

    outDevice.vkGetInstanceProcAddr = fp_vkGetInstanceProcAddr;
    volkInitializeCustom(fp_vkGetInstanceProcAddr);
#else
    // Windows / Linux
    if (volkInitialize() != VK_SUCCESS) {
        return { std::move(outDevice), CustomError(ErrorCode::InstanceCreationFailed, "Failed to initialize volk") };
    }
#endif

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

    volkLoadInstance(outDevice.Instance);

    // 3. Physical device selection.
    vkb::PhysicalDeviceSelector selector { outDevice.vkbInstance };

    auto phys_builder = selector.set_minimum_version(
                                    config.MinVulkanVersion.Major,
                                    config.MinVulkanVersion.Minor)
                            .require_present(config.RequirePresent);

    std::vector<const char*> requiredExts;
    if (config.DeviceExtensions.empty()) {
        requiredExts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    } else {
        requiredExts = config.DeviceExtensions;
    }

    for (const char* ext : requiredExts) {
        phys_builder = phys_builder.add_required_extension(ext);
    }

    vkb::PhysicalDevice physicalDevice;

    if (config.TargetDeviceIndex.has_value()) {
        auto devices_ret = phys_builder.select_devices();
        if (!devices_ret) {
            return { std::move(outDevice),
                CustomError(ErrorCode::PhysicalDeviceSelectionFailed,
                    "Failed to enumerate physical devices") };
        }
        auto devices = devices_ret.value();
        if (config.TargetDeviceIndex.value() >= devices.size()) {
            return { std::move(outDevice),
                CustomError(ErrorCode::PhysicalDeviceSelectionFailed,
                    "TargetDeviceIndex is out of range") };
        }

        physicalDevice = devices[config.TargetDeviceIndex.value()];
    } else {
        auto gpuPreference = config.PreferIntegratedGPU
            ? vkb::PreferredDeviceType::integrated
            : vkb::PreferredDeviceType::discrete;

        phys_builder = phys_builder.prefer_gpu_device_type(gpuPreference);
        auto phys_ret = phys_builder.select();
        if (!phys_ret) {
            return { std::move(outDevice),
                CustomError(ErrorCode::PhysicalDeviceSelectionFailed,
                    "Failed to select physical device: " + phys_ret.error().message()) };
        }
        physicalDevice = phys_ret.value();
    }

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

    volkLoadDevice(outDevice.LogicalDevice);

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

    // 6. VMA allocator.
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo {};
    allocatorInfo.physicalDevice = outDevice.PhysicalDevice;
    allocatorInfo.device = outDevice.LogicalDevice;
    allocatorInfo.instance = outDevice.Instance;
    allocatorInfo.vulkanApiVersion = config.MinVulkanVersion.Pack();
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;

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
    vkGetInstanceProcAddr = other.vkGetInstanceProcAddr;

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
    vkGetInstanceProcAddr = other.vkGetInstanceProcAddr;

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

uint32_t GetAvailableDeviceCount()
{
#ifdef __APPLE__
    void* moltenVkModule = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!moltenVkModule)
        return 0;
    auto fp_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(moltenVkModule, "vkGetInstanceProcAddr");
    if (!fp_vkGetInstanceProcAddr)
        return 0;
    volkInitializeCustom(fp_vkGetInstanceProcAddr);
#else
    if (volkInitialize() != VK_SUCCESS)
        return 0;
#endif

    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("QueryCount")
                        .request_validation_layers(false)
                        .build();
    if (!inst_ret)
        return 0;

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(inst_ret.value().instance, &count, nullptr);

    vkb::destroy_instance(inst_ret.value());
    return count;
}

} // namespace yst::core

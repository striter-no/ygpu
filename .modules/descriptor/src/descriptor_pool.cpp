#include <descriptor/descriptor_pool.hpp>

namespace yst::core {

DescriptorPoolConfig CreateConfig(DescriptorPoolPreset preset)
{
    DescriptorPoolConfig cfg;
    switch (preset) {
    case DescriptorPoolPreset::Manual:
        cfg.PoolSizes.clear();
        return cfg;
    case DescriptorPoolPreset::Default:
    default:
        return cfg;
    }
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
{
    device_ = other.device_;
    pool = other.pool;
    config = std::move(other.config);
    other.device_ = nullptr;
    other.pool = VK_NULL_HANDLE;
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
{
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        pool = other.pool;
        config = std::move(other.config);
        other.device_ = nullptr;
        other.pool = VK_NULL_HANDLE;
    }
    return *this;
}

void DescriptorPool::Destroy()
{
    if (pool != VK_NULL_HANDLE && device_) {
        vkDestroyDescriptorPool(device_->LogicalDevice, pool, nullptr);
        pool = VK_NULL_HANDLE;
        device_ = nullptr;
    }
}

CustomError DescriptorPool::Reset()
{
    if (pool == VK_NULL_HANDLE || !device_)
        return CustomError();
    if (vkResetDescriptorPool(device_->LogicalDevice, pool, 0) != VK_SUCCESS) {
        return CustomError(ErrorCode::DescriptorPoolExhausted,
            "Failed to reset descriptor pool");
    }
    return CustomError();
}

std::pair<DescriptorPool, CustomError> CreateDescriptorPool(
    Device& device, const DescriptorPoolConfig& config)
{
    DescriptorPool out;
    out.device_ = &device;
    out.config = config;

    if (config.MaxSets == 0) {
        return { std::move(out),
            CustomError(ErrorCode::DescriptorPoolCreationFailed,
                "MaxSets must be > 0") };
    }

    VkDescriptorPoolCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = config.Flags;
    info.maxSets = config.MaxSets;
    info.poolSizeCount = static_cast<uint32_t>(config.PoolSizes.size());
    info.pPoolSizes = config.PoolSizes.data();

    if (vkCreateDescriptorPool(device.LogicalDevice, &info, nullptr, &out.pool)
        != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::DescriptorPoolCreationFailed,
                "Failed to create descriptor pool") };
    }

    return { std::move(out), CustomError() };
}

} // namespace yst::core


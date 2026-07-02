#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <descriptor/bind_group_layout.hpp>
#include <device/device.hpp>
#include <errors.hpp>

namespace yst::core {

struct DescriptorPoolConfig {
    uint32_t MaxSets = 1000;
    std::vector<VkDescriptorPoolSize> PoolSizes;
    VkDescriptorPoolCreateFlags Flags
        = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    DescriptorPoolConfig& AutoSizeFromLayouts(
        const std::vector<const BindGroupLayout*>& layouts)
    {
        PoolSizes.clear();
        for (const auto* layout : layouts) {
            if (layout)
                layout->AccumulatePoolSizes(PoolSizes, MaxSets);
        }
        return *this;
    }
};

/// Owning wrapper around a VkDescriptorPool. RAII. Destroying the pool
/// invalidates all BindGroups allocated from it.
class DescriptorPool {
public:
    VkDescriptorPool pool = VK_NULL_HANDLE;
    DescriptorPoolConfig config;

    DescriptorPool() = default;
    ~DescriptorPool() { Destroy(); }

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;
    DescriptorPool(DescriptorPool&& other) noexcept;
    DescriptorPool& operator=(DescriptorPool&& other) noexcept;

    void Destroy();
    CustomError Reset();

private:
    Device* device_ = nullptr;

    friend std::pair<DescriptorPool, CustomError> CreateDescriptorPool(
        Device& device, const DescriptorPoolConfig& config);
};

std::pair<DescriptorPool, CustomError> CreateDescriptorPool(
    Device& device, const DescriptorPoolConfig& config);

} // namespace yst::core

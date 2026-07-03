// Test: yst::core::BindGroupLayout + BindingLayoutEntry helpers
//
// Coverage:
//   1. ToVkDescriptorType maps each BindingType correctly.
//   2. AccumulatePoolSizes aggregates counts per descriptor type.
//   3. (Integration) CreateBindGroupLayout with one UBO + one sampler
//      succeeds on a real device.
//
// Run: ./test_bind_group_layout
#include <iostream>

#include <volk.h>

#include <descriptor/bind_group_layout.hpp>
#include <device/device.hpp>
#include <errors.hpp>

#include "tests/device_helper.hpp"

static int test_descriptor_type_mapping()
{
    using BT = yst::core::BindingType;
    auto check = [](BT bt, VkDescriptorType expected) -> int {
        if (yst::core::ToVkDescriptorType(bt) != expected) {
            std::cerr << "FAIL: BindingType " << static_cast<int>(bt)
                      << " mapped to wrong VkDescriptorType\n";
            return 1;
        }
        return 0;
    };

    if (int rc = check(BT::UniformBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER))
        return rc;
    if (int rc = check(BT::StorageBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER))
        return rc;
    if (int rc = check(BT::ReadonlyStorageBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER))
        return rc;
    if (int rc = check(BT::SampledTexture, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE))
        return rc;
    if (int rc = check(BT::StorageTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE))
        return rc;
    if (int rc = check(BT::Sampler, VK_DESCRIPTOR_TYPE_SAMPLER))
        return rc;
    if (int rc = check(BT::CombinedTextureSampler,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER))
        return rc;
    return 0;
}

static int test_pool_size_aggregation()
{
    using namespace yst::core;

    BindGroupLayoutConfig cfg;
    BindingLayoutEntry e;
    e.AsUniformBuffer(0); // 1 UBO
    cfg.Entries.push_back(e);
    e.AsUniformBuffer(1); // another UBO
    cfg.Entries.push_back(e);
    e.AsSampler(2); // 1 sampler
    cfg.Entries.push_back(e);

    // Build a fake layout (we don't need a real VkDescriptorSetLayout to
    // exercise AccumulatePoolSizes — it only reads the config).
    BindGroupLayout layout;
    layout.config = cfg;

    std::vector<VkDescriptorPoolSize> sizes;
    layout.AccumulatePoolSizes(sizes, /*setCount=*/10);

    // Expect:
    //   - UBO: 2 bindings * 10 sets = 20 descriptors
    //   - Sampler: 1 binding * 10 sets = 10 descriptors
    if (sizes.size() != 2) {
        std::cerr << "FAIL: expected 2 pool sizes, got " << sizes.size() << "\n";
        return 1;
    }
    uint32_t uboCount = 0, samplerCount = 0;
    for (const auto& s : sizes) {
        if (s.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            uboCount = s.descriptorCount;
        else if (s.type == VK_DESCRIPTOR_TYPE_SAMPLER)
            samplerCount = s.descriptorCount;
    }
    if (uboCount != 20) {
        std::cerr << "FAIL: UBO count " << uboCount << " != 20\n";
        return 2;
    }
    if (samplerCount != 10) {
        std::cerr << "FAIL: sampler count " << samplerCount << " != 10\n";
        return 3;
    }
    return 0;
}

static int test_create_bind_group_layout_integration()
{
    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    using namespace yst::core;

    BindGroupLayoutConfig cfg;
    BindingLayoutEntry e;
    e.AsUniformBuffer(0);
    cfg.Entries.push_back(e);
    e.AsSampledTexture(1);
    cfg.Entries.push_back(e);
    e.AsSampler(2);
    cfg.Entries.push_back(e);

    auto [layout, err] = CreateBindGroupLayout(*device, cfg);
    if (err) {
        std::cerr << "FAIL: " << err.str() << "\n";
        return 1;
    }
    if (layout.layout == VK_NULL_HANDLE) {
        std::cerr << "FAIL: layout handle is null\n";
        return 2;
    }

    layout.Destroy();
    if (layout.layout != VK_NULL_HANDLE) {
        std::cerr << "FAIL: Destroy did not null the handle\n";
        return 3;
    }
    return 0;
}

int main()
{
    int failures = 0;
    int rc;

    if ((rc = test_descriptor_type_mapping()) != 0) {
        std::cerr << "test_descriptor_type_mapping failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_pool_size_aggregation()) != 0) {
        std::cerr << "test_pool_size_aggregation failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_create_bind_group_layout_integration()) != 0) {
        std::cerr << "test_create_bind_group_layout_integration failed: " << rc
                  << "\n";
        failures++;
    }

    if (failures > 0) {
        std::cerr << "test_bind_group_layout: " << failures
                  << " test(s) failed\n";
        return 1;
    }
    std::cout << "test_bind_group_layout: all tests passed\n";
    return 0;
}

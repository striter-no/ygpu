// Test: yst::core::DescriptorPool + DescriptorPoolConfig::AutoSizeFromLayouts
//
// Coverage:
//   1. CreateDescriptorPool rejects MaxSets == 0.
//   2. AutoSizeFromLayouts accumulates pool sizes correctly across multiple
//      layouts.
//   3. (Integration) Create + Reset + Destroy on a real device.
//
// Run: ./test_descriptor_pool
#include <iostream>

#include <descriptor/bind_group_layout.hpp>
#include <descriptor/descriptor_pool.hpp>
#include <device/device.hpp>
#include <errors.hpp>

#include "tests/device_helper.hpp"

static int test_create_rejects_zero_maxsets()
{
    yst::core::Device nullDevice;
    yst::core::DescriptorPoolConfig cfg;
    cfg.MaxSets = 0;

    auto [pool, err] = yst::core::CreateDescriptorPool(nullDevice, cfg);
    if (!err) {
        std::cerr << "FAIL: expected error for MaxSets == 0\n";
        return 1;
    }
    if (!err.Is(yst::ErrorCode::DescriptorPoolCreationFailed)) {
        std::cerr << "FAIL: expected DescriptorPoolCreationFailed, got "
                  << err.name() << "\n";
        return 2;
    }
    return 0;
}

static int test_autosize_from_layouts()
{
    using namespace yst::core;

    // Two layouts that share a UBO binding but each also has a sampler.
    BindGroupLayoutConfig cfgA;
    BindingLayoutEntry e;
    e.AsUniformBuffer(0);
    cfgA.Entries.push_back(e);
    e.AsSampler(1);
    cfgA.Entries.push_back(e);

    BindGroupLayoutConfig cfgB;
    e.AsUniformBuffer(0);
    cfgB.Entries.push_back(e);
    e.AsStorageBuffer(1);
    cfgB.Entries.push_back(e);

    // Build fake layouts (only config is read by AccumulatePoolSizes).
    BindGroupLayout a, b;
    a.config = cfgA;
    b.config = cfgB;

    DescriptorPoolConfig poolCfg;
    poolCfg.MaxSets = 5;
    poolCfg.AutoSizeFromLayouts({ &a, &b });

    // Expected pool sizes:
    //   UBO:           1 (layout A) + 1 (layout B) = 2 bindings * 5 sets = 10
    //   Sampler:       1 (layout A) * 5 sets = 5
    //   StorageBuffer: 1 (layout B) * 5 sets = 5
    if (poolCfg.PoolSizes.size() != 3) {
        std::cerr << "FAIL: expected 3 pool sizes, got "
                  << poolCfg.PoolSizes.size() << "\n";
        return 1;
    }

    auto findCount = [&](VkDescriptorType t) -> uint32_t {
        for (const auto& s : poolCfg.PoolSizes)
            if (s.type == t)
                return s.descriptorCount;
        return 0;
    };

    if (findCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) != 10) {
        std::cerr << "FAIL: UBO pool size != 10\n";
        return 2;
    }
    if (findCount(VK_DESCRIPTOR_TYPE_SAMPLER) != 5) {
        std::cerr << "FAIL: sampler pool size != 5\n";
        return 3;
    }
    if (findCount(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) != 5) {
        std::cerr << "FAIL: storage pool size != 5\n";
        return 4;
    }
    return 0;
}

static int test_create_reset_destroy_integration()
{
    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    using namespace yst::core;

    // Build a real BindGroupLayout so AutoSizeFromLayouts has something
    // to size against.
    BindGroupLayoutConfig cfg;
    BindingLayoutEntry e;
    e.AsUniformBuffer(0);
    cfg.Entries.push_back(e);
    auto [layout, layoutErr] = CreateBindGroupLayout(*device, cfg);
    if (layoutErr) {
        std::cerr << "FAIL: " << layoutErr.str() << "\n";
        return 1;
    }

    DescriptorPoolConfig poolCfg;
    poolCfg.MaxSets = 4;
    poolCfg.AutoSizeFromLayouts({ &layout });

    auto [pool, poolErr] = CreateDescriptorPool(*device, poolCfg);
    if (poolErr) {
        std::cerr << "FAIL: " << poolErr.str() << "\n";
        return 2;
    }
    if (pool.pool == VK_NULL_HANDLE) {
        std::cerr << "FAIL: pool handle is null\n";
        return 3;
    }

    if (auto resetErr = pool.Reset()) {
        std::cerr << "FAIL: reset failed: " << resetErr.str() << "\n";
        return 4;
    }

    pool.Destroy();
    layout.Destroy();
    if (pool.pool != VK_NULL_HANDLE) {
        std::cerr << "FAIL: Destroy did not null pool handle\n";
        return 5;
    }
    return 0;
}

int main()
{
    int failures = 0;
    int rc;

    if ((rc = test_create_rejects_zero_maxsets()) != 0) {
        std::cerr << "test_create_rejects_zero_maxsets failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_autosize_from_layouts()) != 0) {
        std::cerr << "test_autosize_from_layouts failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_create_reset_destroy_integration()) != 0) {
        std::cerr << "test_create_reset_destroy_integration failed: " << rc
                  << "\n";
        failures++;
    }

    if (failures > 0) {
        std::cerr << "test_descriptor_pool: " << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "test_descriptor_pool: all tests passed\n";
    return 0;
}


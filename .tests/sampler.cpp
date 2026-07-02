// Test: yst::core::Sampler
//
// Coverage:
//   1. (Integration) Default SamplerConfig creates a working sampler.
//   2. (Integration) Anisotropic sampler (with device-supported maxAniso).
//   3. (Integration) Sampler with compare op (for shadow maps).
//
// Run: ./test_sampler
#include <iostream>

#include <device/device.hpp>
#include <errors.hpp>
#include <image/sampler.hpp>

#include "tests/device_helper.hpp"

static int test_default_sampler()
{
    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    auto [sampler, err] = yst::core::CreateSampler(*device);
    if (err) {
        std::cerr << "FAIL: " << err.str() << "\n";
        return 1;
    }
    if (!sampler.IsValid()) {
        std::cerr << "FAIL: sampler handle is null\n";
        return 2;
    }

    sampler.Destroy();
    if (sampler.IsValid()) {
        std::cerr << "FAIL: Destroy did not null handle\n";
        return 3;
    }
    return 0;
}

static int test_anisotropic_sampler()
{
    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    // Check device supports anisotropic filtering; skip if not.
    VkPhysicalDeviceFeatures features {};
    vkGetPhysicalDeviceFeatures(device->PhysicalDevice, &features);
    if (!features.samplerAnisotropy) {
        std::cout << "[skip] device does not support samplerAnisotropy\n";
        return 0;
    }

    auto cfg = yst::core::SamplerBuilder(yst::core::SamplerPreset::LinearRepeat)
                   .WithAnisotropy(true, 8.0f)
                   .Build();

    auto [sampler, err] = yst::core::CreateSampler(*device, cfg);
    if (err) {
        std::cerr << "FAIL: " << err.str() << "\n";
        return 1;
    }

    sampler.Destroy();
    return 0;
}

static int test_compare_sampler()
{
    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    auto cfg = yst::core::SamplerBuilder(yst::core::SamplerPreset::ShadowCompare)
                   .WithCompare(true, VK_COMPARE_OP_LESS)
                   .WithLodRange(0.0f, 1.0f)
                   .Build();

    auto [sampler, err] = yst::core::CreateSampler(*device, cfg);
    if (err) {
        std::cerr << "FAIL: " << err.str() << "\n";
        return 1;
    }

    sampler.Destroy();
    return 0;
}

int main()
{
    int failures = 0;
    int rc;

    if ((rc = test_default_sampler()) != 0) {
        std::cerr << "test_default_sampler failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_anisotropic_sampler()) != 0) {
        std::cerr << "test_anisotropic_sampler failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_compare_sampler()) != 0) {
        std::cerr << "test_compare_sampler failed: " << rc << "\n";
        failures++;
    }

    if (failures > 0) {
        std::cerr << "test_sampler: " << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "test_sampler: all tests passed\n";
    return 0;
}


// Test: yst::core::Image + ImageView
//
// Coverage:
//   1. CreateImage rejects zero-sized extent.
//   2. CreateImageView rejects a null image.
//   3. (Integration) CreateImage + CreateImageView + Destroy on a real device.
//
// Run: ./test_image
#include <iostream>

#include <device/device.hpp>
#include <errors.hpp>
#include <image/image.hpp>

#include "tests/device_helper.hpp"

static int test_create_rejects_zero_extent()
{
    yst::core::Device nullDevice;
    yst::core::ImageConfig cfg;
    cfg.Extent = { 0, 0, 0 };

    auto [img, err] = yst::core::CreateImage(nullDevice, cfg);
    if (!err) {
        std::cerr << "FAIL: expected error for zero-sized extent\n";
        return 1;
    }
    if (!err.Is(yst::ErrorCode::ImageCreationFailed)) {
        std::cerr << "FAIL: expected ImageCreationFailed, got " << err.name()
                  << "\n";
        return 2;
    }
    return 0;
}

static int test_create_view_rejects_null_image()
{
    yst::core::Device nullDevice;
    yst::core::Image nullImage;
    yst::core::ImageViewConfig cfg;

    auto [view, err] = yst::core::CreateImageView(nullDevice, nullImage, cfg);
    if (!err) {
        std::cerr << "FAIL: expected error for null image\n";
        return 1;
    }
    if (!err.Is(yst::ErrorCode::ImageViewCreationFailed)) {
        std::cerr << "FAIL: expected ImageViewCreationFailed, got "
                  << err.name() << "\n";
        return 2;
    }
    return 0;
}

static int test_full_lifecycle_integration()
{
    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    yst::core::ImageConfig cfg;
    cfg.As2DTexture(64, 64);
    auto [image, imgErr] = yst::core::CreateImage(*device, cfg);
    if (imgErr) {
        std::cerr << "FAIL: image creation: " << imgErr.str() << "\n";
        return 1;
    }
    if (!image.IsValid()) {
        std::cerr << "FAIL: image handle is null\n";
        return 2;
    }
    if (image.extent.width != 64 || image.extent.height != 64) {
        std::cerr << "FAIL: extent mismatch\n";
        return 3;
    }
    if (image.mipLevels != 1) {
        std::cerr << "FAIL: mipLevels mismatch\n";
        return 4;
    }

    auto [view, viewErr] = yst::core::CreateImageView(*device, image);
    if (viewErr) {
        std::cerr << "FAIL: view creation: " << viewErr.str() << "\n";
        return 5;
    }
    if (!view.IsValid()) {
        std::cerr << "FAIL: view handle is null\n";
        return 6;
    }

    view.Destroy(*device);
    image.Destroy(*device);
    if (image.IsValid()) {
        std::cerr << "FAIL: Destroy did not null image handle\n";
        return 7;
    }
    if (view.IsValid()) {
        std::cerr << "FAIL: Destroy did not null view handle\n";
        return 8;
    }
    return 0;
}

int main()
{
    int failures = 0;
    int rc;

    if ((rc = test_create_rejects_zero_extent()) != 0) {
        std::cerr << "test_create_rejects_zero_extent failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_create_view_rejects_null_image()) != 0) {
        std::cerr << "test_create_view_rejects_null_image failed: " << rc
                  << "\n";
        failures++;
    }
    if ((rc = test_full_lifecycle_integration()) != 0) {
        std::cerr << "test_full_lifecycle_integration failed: " << rc << "\n";
        failures++;
    }

    if (failures > 0) {
        std::cerr << "test_image: " << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "test_image: all tests passed\n";
    return 0;
}

// Test: yst::core::Texture2D + LoadStbImage
//
// Coverage:
//   1. CreateTexture2D rejects null pixels.
//   2. CreateTexture2D rejects zero extent.
//   3. LoadStbImage on a missing file returns TextureFileOpenFailed.
//   4. (Integration) Create a Texture2D from a synthetic RGBA8 pixel
//      buffer (no real image file needed).
//   5. (Integration) Create a Texture2D with mipmap generation.
//   6. (Integration, optional) LoadStbImage + CreateTexture2D from a real
//      PNG. Skipped if the stb_image shim is in use or the file is missing.
//
// Run: ./test_texture
#include <iostream>
#include <vector>

#include <device/device.hpp>
#include <errors.hpp>
#include <image/image.hpp>
#include <image/sampler.hpp>
#include <texture/texture.hpp>
#include <texture/texture_loader.hpp>

#include "tests/device_helper.hpp"

static int test_create_rejects_null_pixels()
{
    yst::core::Device nullDevice;
    auto cfg = yst::core::Texture2DBuilder(yst::core::Texture2DPreset::Rgba8Unorm)
                   .WithPixels(nullptr)
                   .WithExtent(16, 16)
                   .Build();

    auto [tex, err] = yst::core::CreateTexture2D(nullDevice, cfg);
    if (!err) {
        std::cerr << "FAIL: expected error for null pixels\n";
        return 1;
    }
    if (!err.Is(yst::ErrorCode::TextureUploadFailed)) {
        std::cerr << "FAIL: expected TextureUploadFailed, got " << err.name()
                  << "\n";
        return 2;
    }
    return 0;
}

static int test_create_rejects_zero_extent()
{
    yst::core::Device nullDevice;
    static const uint8_t dummy[4] = { 0, 0, 0, 0 };
    auto cfg = yst::core::Texture2DBuilder(yst::core::Texture2DPreset::Rgba8Unorm)
                   .WithPixels(dummy)
                   .WithExtent(0, 0)
                   .Build();

    auto [tex, err] = yst::core::CreateTexture2D(nullDevice, cfg);
    if (!err) {
        std::cerr << "FAIL: expected error for zero extent\n";
        return 1;
    }
    return 0;
}

static int test_load_missing_file()
{
    auto [pixels, err] = yst::core::LoadStbImage("/nonexistent/texture.png");
    if (!err) {
        std::cerr << "FAIL: expected error for missing file\n";
        return 1;
    }
    if (!err.Is(yst::ErrorCode::TextureFileOpenFailed)) {
        std::cerr << "FAIL: expected TextureFileOpenFailed, got "
                  << err.name() << "\n";
        return 2;
    }
    return 0;
}

static int test_synthetic_texture_integration()
{
    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    // 32x32 RGBA8 gradient pattern.
    constexpr uint32_t W = 32, H = 32;
    std::vector<uint8_t> pixels(W * H * 4);
    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            const size_t i = (y * W + x) * 4;
            pixels[i + 0] = static_cast<uint8_t>(x * 8);
            pixels[i + 1] = static_cast<uint8_t>(y * 8);
            pixels[i + 2] = 128;
            pixels[i + 3] = 255;
        }
    }

    auto cfg = yst::core::Texture2DBuilder(yst::core::Texture2DPreset::Rgba8Unorm)
                   .WithPixels(pixels.data())
                   .WithExtent(W, H)
                   .Build();

    auto [tex, err] = yst::core::CreateTexture2D(*device, cfg);
    if (err) {
        std::cerr << "FAIL: " << err.str() << "\n";
        return 1;
    }
    if (!tex.image.IsValid() || !tex.view.IsValid() || !tex.sampler.IsValid()) {
        std::cerr << "FAIL: texture components not all valid\n";
        return 2;
    }

    tex.Destroy();
    if (tex.image.IsValid() || tex.view.IsValid() || tex.sampler.IsValid()) {
        std::cerr << "FAIL: Destroy did not null handles\n";
        return 3;
    }
    return 0;
}

static int test_mipmapped_texture_integration()
{
    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    // 64x64 solid color — mipmap generation should still work.
    constexpr uint32_t W = 64, H = 64;
    std::vector<uint8_t> pixels(W * H * 4, 0xFF); // white

    auto cfg = yst::core::Texture2DBuilder(yst::core::Texture2DPreset::Rgba8Unorm)
                   .WithPixels(pixels.data())
                   .WithExtent(W, H)
                   .WithMipmaps(true)
                   .Build();

    auto [tex, err] = yst::core::CreateTexture2D(*device, cfg);
    if (err) {
        std::cerr << "FAIL: " << err.str() << "\n";
        return 1;
    }

    // 64x64 → 7 mip levels (64, 32, 16, 8, 4, 2, 1).
    if (tex.image.mipLevels != 7) {
        std::cerr << "FAIL: expected 7 mip levels, got " << tex.image.mipLevels
                  << "\n";
        return 2;
    }

    tex.Destroy();
    return 0;
}

static int test_real_png_load_integration()
{
    // Try to load a real PNG; skip if the stb_image shim is in use (the
    // shim always returns nullptr from stbi_load).
    auto [pixels, loadErr] = yst::core::LoadStbImage("./assets/textures/checkerboard.png");
    if (loadErr) {
        std::cout << "[skip] real PNG load failed (likely stb_image shim): "
                  << loadErr.str() << "\n";
        return 0;
    }

    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    auto cfg = yst::core::Texture2DBuilder(yst::core::Texture2DPreset::MipmappedRgba8Srgb)
                   .WithPixels(pixels.bytes.data())
                   .WithExtent(static_cast<uint32_t>(pixels.Width),
                       static_cast<uint32_t>(pixels.Height))
                   .Build();

    auto [tex, err] = yst::core::CreateTexture2D(*device, cfg);
    if (err) {
        std::cerr << "FAIL: " << err.str() << "\n";
        return 1;
    }

    tex.Destroy();
    return 0;
}

int main()
{
    int failures = 0;
    int rc;

    if ((rc = test_create_rejects_null_pixels()) != 0) {
        std::cerr << "test_create_rejects_null_pixels failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_create_rejects_zero_extent()) != 0) {
        std::cerr << "test_create_rejects_zero_extent failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_load_missing_file()) != 0) {
        std::cerr << "test_load_missing_file failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_synthetic_texture_integration()) != 0) {
        std::cerr << "test_synthetic_texture_integration failed: " << rc
                  << "\n";
        failures++;
    }
    if ((rc = test_mipmapped_texture_integration()) != 0) {
        std::cerr << "test_mipmapped_texture_integration failed: " << rc
                  << "\n";
        failures++;
    }
    if ((rc = test_real_png_load_integration()) != 0) {
        std::cerr << "test_real_png_load_integration failed: " << rc << "\n";
        failures++;
    }

    if (failures > 0) {
        std::cerr << "test_texture: " << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "test_texture: all tests passed\n";
    return 0;
}

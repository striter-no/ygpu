// Test: yst::core::ShaderModule + LoadSpvFile
//
// Coverage:
//   1. LoadSpvFile on a non-existent path returns ShaderFileOpenFailed.
//   2. CreateShaderModule rejects empty SPIR-V with ShaderFileEmpty.
//   3. (Integration) If a real .spv file is present in tests/assets/,
//      CreateShaderModule succeeds and Destroy cleans up.
//
// Run: ./test_shader_module
#include <filesystem>
#include <iostream>
#include <vector>

#include <device/device.hpp>
#include <errors.hpp>
#include <shader/shader.hpp>

#include "tests/device_helper.hpp"
#include "tests/shaders_helper.hpp"

namespace fs = std::filesystem;

static int test_loadspvfile_missing()
{
    auto [spv, err] = yst::core::LoadSpvFile("/nonexistent/path/to/shader.spv");
    if (!err) {
        std::cerr << "FAIL: expected error for missing file\n";
        return 1;
    }
    if (!err.Is(yst::ErrorCode::ShaderFileOpenFailed)) {
        std::cerr << "FAIL: expected ShaderFileOpenFailed, got " << err.name()
                  << " (" << err.str() << ")\n";
        return 2;
    }
    if (!spv.empty()) {
        std::cerr << "FAIL: expected empty SPIR-V on error\n";
        return 3;
    }
    return 0;
}

static int test_create_shader_module_rejects_empty()
{
    // No device needed — CreateShaderModule validates config before
    // touching Vulkan, so passing a null device is fine for this path.
    yst::core::Device nullDevice;
    auto cfg = yst::core::ShaderModuleBuilder(yst::core::ShaderModulePreset::Vertex)
                   .Build();
    // Leave Spirv empty.

    auto [mod, err] = yst::core::CreateShaderModule(nullDevice, cfg);
    if (!err) {
        std::cerr << "FAIL: expected error for empty SPIR-V\n";
        return 1;
    }
    if (!err.Is(yst::ErrorCode::ShaderFileEmpty)) {
        std::cerr << "FAIL: expected ShaderFileEmpty, got " << err.name() << "\n";
        return 2;
    }
    if (mod.module != VK_NULL_HANDLE) {
        std::cerr << "FAIL: module handle should be null on error\n";
        return 3;
    }
    return 0;
}

static int test_create_shader_module_integration()
{
    auto maybeSpv = yst::test::shaders::TryLoadTestShader("minimal.vert.spv");
    if (!maybeSpv) {
        std::cout << "[skip] no tests/assets/minimal.vert.spv found\n";
        return 0;
    }

    auto device = yst::test::TryCreateTestDevice();
    if (!device) {
        std::cout << "[skip] no Vulkan device available\n";
        return 0;
    }

    auto cfg = yst::core::ShaderModuleBuilder(yst::core::ShaderModulePreset::Vertex)
                   .WithSpirv(std::move(*maybeSpv))
                   .WithEntryPoint("main")
                   .Build();

    auto [mod, err] = yst::core::CreateShaderModule(*device, cfg);
    if (err) {
        std::cerr << "FAIL: " << err.str() << "\n";
        return 1;
    }
    if (mod.module == VK_NULL_HANDLE) {
        std::cerr << "FAIL: module handle is null despite no error\n";
        return 2;
    }
    if (mod.stage != VK_SHADER_STAGE_VERTEX_BIT) {
        std::cerr << "FAIL: stage mismatch\n";
        return 3;
    }
    if (mod.entryPoint != "main") {
        std::cerr << "FAIL: entry point mismatch\n";
        return 4;
    }

    // ToStageCreateInfo should produce a usable struct.
    auto stageInfo = mod.ToStageCreateInfo();
    if (stageInfo.module != mod.module
        || stageInfo.stage != VK_SHADER_STAGE_VERTEX_BIT
        || std::string(stageInfo.pName) != "main") {
        std::cerr << "FAIL: ToStageCreateInfo produced wrong fields\n";
        return 5;
    }

    mod.Destroy();
    if (mod.module != VK_NULL_HANDLE) {
        std::cerr << "FAIL: Destroy did not null the handle\n";
        return 6;
    }
    return 0;
}

int main()
{
    int failures = 0;
    int rc;

    if ((rc = test_loadspvfile_missing()) != 0) {
        std::cerr << "test_loadspvfile_missing failed: " << rc << "\n";
        failures++;
    }
    if ((rc = test_create_shader_module_rejects_empty()) != 0) {
        std::cerr << "test_create_shader_module_rejects_empty failed: " << rc
                  << "\n";
        failures++;
    }
    if ((rc = test_create_shader_module_integration()) != 0) {
        std::cerr << "test_create_shader_module_integration failed: " << rc
                  << "\n";
        failures++;
    }

    if (failures > 0) {
        std::cerr << "test_shader_module: " << failures
                  << " test(s) failed\n";
        return 1;
    }
    std::cout << "test_shader_module: all tests passed\n";
    return 0;
}


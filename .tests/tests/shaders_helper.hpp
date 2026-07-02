#pragma once
//
// Test shader helpers.
//
// Two strategies:
//   1. DummySpv() — returns a tiny non-empty vector for unit tests that
//      only need to exercise ShaderModuleConfig validation (no real
//      GPU shader is created). The bytes are not valid SPIR-V, so they
//      must NOT be passed to CreateShaderModule.
//
//   2. TryLoadTestShader(name) — integration tests that need a real
//      shader load ./tests/assets/<name> via LoadSpvFile. Returns
//      std::nullopt if the file doesn't exist; tests should skip (return 0)
//      in that case so CI without compiled shaders doesn't fail.
//
//      To populate the assets directory, compile the .glsl sources in
//      tests/assets/src/ using glslangValidator or shaderc:
//
//          glslangValidator -V tests/assets/src/triangle.vert.glsl \\
//                            -o tests/assets/triangle.vert.spv
//
#include <shader/shader.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace yst::test::shaders {

/// Returns a tiny non-empty SPIR-V byte vector. The contents are NOT a
/// valid shader — use only for config-validation tests that never reach
/// vkCreateShaderModule.
inline std::vector<uint32_t> DummySpv()
{
    // 16 dwords of zeros. Real SPIR-V starts with magic 0x07230203 —
    // a VkShaderModule created from this WILL fail validation, which is
    // exactly what we want for negative tests.
    return std::vector<uint32_t>(16, 0x07230203u);
}

/// Try to load a compiled .spv file from the test assets directory.
/// Looks at: ./tests/assets/<name>
/// Returns std::nullopt if the file doesn't exist (test should skip).
inline std::optional<std::vector<uint32_t>> TryLoadTestShader(
    const std::string& name)
{
    namespace fs = std::filesystem;
    const fs::path candidates[] = {
        fs::path("tests/assets") / name,
        fs::path("./tests/assets") / name,
        fs::path("../tests/assets") / name,
    };
    for (const auto& p : candidates) {
        if (fs::exists(p)) {
            auto [spv, err] = yst::core::LoadSpvFile(p.string());
            if (!err)
                return std::move(spv);
            return std::nullopt;
        }
    }
    return std::nullopt;
}

} // namespace yst::test::shaders

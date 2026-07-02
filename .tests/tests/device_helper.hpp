#pragma once
//
// Shared helper for integration tests that need a real Vulkan device.
// Tests that can run without a GPU should call TryCreateTestDevice() and
// skip (return 0) when it returns std::nullopt.
//
// Usage:
//   #include <test/device_helper.hpp>
//   int main() {
//       auto device = yst::test::TryCreateTestDevice();
//       if (!device) {
//           std::cout << "[skip] no Vulkan device\n";
//           return 0;
//       }
//       // ... real test using device.value() ...
//   }
//
#include <device/device.hpp>
#include <iostream>
#include <optional>

namespace yst::test {

/// Create a Device suitable for tests: HEADLESS preset (so it works on
/// machines without a display), validation off (so test output isn't
/// flooded with layer messages). Returns std::nullopt if no Vulkan device
/// is available — tests should treat this as a skip, not a failure.
inline std::optional<yst::core::Device> TryCreateTestDevice()
{
    auto config = yst::core::CreateConfig(yst::gpuc::HEADLESS_CONFIG);
    config.EnableDebug = false;

    auto [device, err] = yst::core::CreateDevice(config);
    if (err) {
        std::cerr << "[test helper] device creation failed: " << err.str()
                  << "\n";
        return std::nullopt;
    }
    return std::move(device);
}

} // namespace yst::test

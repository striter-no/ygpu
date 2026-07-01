#pragma once

namespace yst::gpuc {
enum Preset { DEFAULT_CONFIG,
    HEADLESS_CONFIG };
enum Backend { BACKEND_VULKAN,
    BACKEND_MOCK };
} // namespace yst::gpuc

namespace yst::core {

struct DeviceConfig {
    bool PreferIntegratedGPU = false;
    yst::gpuc::Backend PreferedBackend = yst::gpuc::BACKEND_VULKAN;
    bool EnableDebug = true;
};

DeviceConfig CreateConfig(yst::gpuc::Preset preset);
} // namespace yst::core

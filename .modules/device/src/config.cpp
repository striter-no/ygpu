#include "device/config.hpp"

namespace yst::core {
DeviceConfig CreateConfig(yst::gpuc::Preset preset)
{
    DeviceConfig config;
    if (preset == gpuc::DEFAULT_CONFIG) {
        config.PreferIntegratedGPU = false;
        config.EnableDebug = true;
    } else if (preset == gpuc::HEADLESS_CONFIG) {
        config.PreferIntegratedGPU = true;
        config.EnableDebug = false;
    }
    return config;
}
} // namespace yst::core

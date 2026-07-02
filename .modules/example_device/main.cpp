#include <device/device.hpp>
#include <exception>
#include <iostream>

int creation_test()
{
    // Use a preset (Layer 1) instead of constructing DeviceConfig by hand —
    // the DEFAULT_CONFIG preset fills in every field with the same values
    // this example used to hardcode. Override individual fields afterwards
    // if desired.
    auto config = yst::core::CreateConfig(yst::gpuc::DEFAULT_CONFIG);
    config.PreferIntegratedGPU = false;
    config.EnableDebug = true;

    auto [device, error] = yst::core::CreateDevice(config);

    if (error) {
        std::cerr << error.str() << std::endl;
        return -1;
    }

    // Use the new accessor instead of poking into vkb internals directly.
    // The underlying vkb::Device is still public for advanced callers, but
    // the named accessor is the supported path.
    std::cout << "GPU Name: " << device.GetDeviceName() << std::endl;

    auto apiVer = device.GetActiveApiVersion();
    std::cout << "Vulkan API: " << apiVer.Major << "." << apiVer.Minor
              << "." << apiVer.Patch << std::endl;

    return 0;
}

int main()
{
    try {
        return creation_test();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }
}


#include <device/device.hpp>
#include <exception>
#include <iostream>

int creation_test()
{
    yst::core::DeviceConfig config;
    config.EnableDebug = true;
    config.PreferIntegratedGPU = false;

    auto [device, error] = yst::core::CreateDevice(config);

    if (error) {
        std::cerr << error.str() << std::endl;
        return -1;
    }

    std::cout << "GPU Name: " << device.vkbDevice.physical_device.name
              << std::endl;
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

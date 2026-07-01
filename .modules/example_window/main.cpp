#include <vulkan/vulkan_core.h>

#include <device/device.hpp>
#include <iostream>
#include <swapchain/swapchain.hpp>
#include <window/window.hpp>

int main() {
    yst::ywin::WindowConfig wconfig;
    wconfig.Width = 1024;
    wconfig.Height = 768;
    wconfig.Title = "YST Window Test";

    auto [window, werr] = yst::ywin::CreateWindow(wconfig);
    if (werr) {
        std::cerr << "Window Error: " << werr.str() << std::endl;
        return -1;
    }

    yst::core::DeviceConfig dconfig;
    dconfig.EnableDebug = true;
    dconfig.PreferIntegratedGPU = false;

    auto [device, derr] = yst::core::CreateDevice(dconfig);
    if (derr) {
        std::cerr << "Device creation error: " << derr.str() << std::endl;
        return -1;
    }

    auto [swapchain, serr] = yst::core::CreateSwapchain(device, window);
    if (serr) {
        std::cerr << "Swapchain creation error: " << serr.str() << std::endl;
        return -1;
    }

    while (window.IsOpen()) {
        window.PollEvents();

        auto [cmd, cmdErr] = swapchain.AcquireNextFrame(window);
        if (cmdErr) {
            if (cmdErr.code == VK_ERROR_OUT_OF_DATE_KHR) {
                swapchain.Recreate(window);
                continue;
            }
            break;
        }

        cmd.BeginRenderPass(
            swapchain.GetRenderPass(), swapchain.GetCurrentFramebuffer(),
            swapchain.GetExtent(), swapchain.GetConfig().ClearColor);

        // ...

        cmd.EndRenderPass();
        swapchain.Present(cmd);
    }

    return 0;
}

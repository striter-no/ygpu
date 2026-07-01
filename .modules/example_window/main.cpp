#include <buffer/buffer.hpp>
#include <device/device.hpp>
#include <errors.hpp>
#include <pipeline/pipeline.hpp>
#include <swapchain/swapchain.hpp>
#include <vulkan/vulkan_core.h>
#include <window/window.hpp>

#include <fstream>
#include <iostream>
#include <vector>

struct Vertex {
    float pos[2];
    float color[3];
};

std::vector<uint32_t> LoadShader(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("failed to open shader: " + path);
    size_t fileSize = (size_t)file.tellg();
    if (fileSize == 0 || fileSize % sizeof(uint32_t) != 0)
        throw std::runtime_error("invalid shader file size: " + path);
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    return buffer;
}

int main()
{
    // Layer 2 config: explicitly fill in the fields we care about. Fields
    // left at their defaults match the historical hardcoded behaviour.
    yst::ywin::WindowConfig wconfig;
    wconfig.Width = 1024;
    wconfig.Height = 768;
    wconfig.Title = "YST Window Test";
    wconfig.Resizable = true;
    auto [window, werr] = yst::ywin::CreateWindow(wconfig);
    if (werr) {
        std::cerr << "Failed to create window: " << werr.str() << std::endl;
        return -1;
    }

    yst::core::DeviceConfig dconfig;
    dconfig.EnableDebug = true;
    dconfig.PreferIntegratedGPU = false;
    dconfig.PreferredBackend = yst::gpuc::BACKEND_VULKAN;
    auto [device, derr] = yst::core::CreateDevice(dconfig);
    if (derr) {
        std::cerr << "Failed to create device: " << derr.str() << std::endl;
        return -1;
    }
    std::cout << "Using GPU: " << device.GetDeviceName() << "\n";

    auto [swapchain, serr] = yst::core::CreateSwapchain(device, window);
    if (serr) {
        std::cerr << "Failed to create swapchain: " << serr.str() << std::endl;
        return -1;
    }

    std::vector<Vertex> vertices = {
        { { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
        { { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } }
    };

    auto [vertexBuffer, berr] = yst::core::CreateBuffer(
        device, sizeof(Vertex) * vertices.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, /*hostVisible=*/true);

    if (berr) {
        std::cerr << "Buffer error: " << berr.str() << "\n";
        return -1;
    }

    if (auto err = vertexBuffer.UploadData(vertices.data(), sizeof(Vertex) * vertices.size())) {
        std::cerr << "Upload error: " << err.str() << "\n";
        return -1;
    }

    yst::core::PipelineConfig pConfig;
    pConfig.renderPass = swapchain.GetRenderPass();
    pConfig.vertexShaderSpv = LoadShader("./assets/shaders/compiled/vert.spv");
    pConfig.fragmentShaderSpv = LoadShader("./assets/shaders/compiled/frag.spv");

    pConfig.bindings.push_back({ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX });
    pConfig.attributes.push_back({ 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos) });
    pConfig.attributes.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });

    // The remaining pipeline state (topology, cull mode, blend, dynamic
    // states, etc.) is left at the defaults baked into PipelineConfig, which
    // match the historical hardcoded pipeline. Override individual fields
    // here to customise.

    auto [pipeline, perr] = yst::core::CreateGraphicsPipeline(device, pConfig);
    if (perr) {
        std::cerr << "Failed to create graphics pipeline: " << perr.str() << std::endl;
        return -1;
    }

    while (window.IsOpen()) {
        window.PollEvents();

        auto [cmd, cmdErr] = swapchain.AcquireNextFrame(window);
        if (cmdErr) {
            if (cmdErr.Is(yst::ErrorCode::SwapchainOutOfDate)
                || swapchain.NeedsRecreate()) {
                swapchain.Recreate(window);
                continue;
            }
            break;
        }

        cmd.BeginRenderPass(swapchain.GetRenderPass(),
            swapchain.GetCurrentFramebuffer(),
            swapchain.GetExtent(),
            swapchain.GetConfig().ClearColor);

        cmd.BindPipeline(pipeline.pipeline);

        cmd.BindVertexBuffer(vertexBuffer.buffer);
        cmd.Draw(vertices.size());

        cmd.EndRenderPass();
        if (auto presentErr = swapchain.Present(cmd)) {
            std::cerr << "Present error: " << presentErr.str() << "\n";
            break;
        }

        // If Present() flagged the swapchain as out-of-date/suboptimal,
        // recreate before the next acquire.
        if (swapchain.NeedsRecreate())
            swapchain.Recreate(window);
    }

    vkDeviceWaitIdle(device.LogicalDevice);
    pipeline.Destroy(device);
    vertexBuffer.Destroy(device);

    return 0;
}

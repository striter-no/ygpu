#include <buffer/buffer.hpp>
#include <device/device.hpp>
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
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    return buffer;
}

int main()
{
    yst::ywin::WindowConfig wconfig { 1024, 768, "YST Window Test", true };
    auto [window, werr] = yst::ywin::CreateWindow(wconfig);
    if (werr) {
        std::cerr << "Failed to create window: " << werr.str() << std::endl;
        return -1;
    }

    yst::core::DeviceConfig dconfig { false, yst::gpuc::BACKEND_VULKAN, true };
    auto [device, derr] = yst::core::CreateDevice(dconfig);
    if (derr) {
        std::cerr << "Failed to create device: " << derr.str() << std::endl;
        return -1;
    }

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
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);

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

    auto [pipeline, perr] = yst::core::CreateGraphicsPipeline(device, pConfig);
    if (perr) {
        std::cerr << "Failed to create graphics pipeline: " << perr.str() << std::endl;
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

        cmd.BeginRenderPass(swapchain.GetRenderPass(), swapchain.GetCurrentFramebuffer(),
            swapchain.GetExtent(), swapchain.GetConfig().ClearColor);

        cmd.BindPipeline(pipeline.pipeline);

        cmd.BindVertexBuffer(vertexBuffer.buffer);
        cmd.Draw(vertices.size());

        cmd.EndRenderPass();
        swapchain.Present(cmd);
    }

    vkDeviceWaitIdle(device.LogicalDevice);
    pipeline.Destroy(device);
    vertexBuffer.Destroy(device);

    return 0;
}

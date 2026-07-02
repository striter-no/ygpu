#include <buffer/buffer.hpp>
#include <descriptor/bind_group.hpp>
#include <descriptor/bind_group_layout.hpp>
#include <descriptor/descriptor_pool.hpp>
#include <descriptor/pipeline_layout.hpp>
#include <device/device.hpp>
#include <errors.hpp>
#include <image/image.hpp>
#include <image/sampler.hpp>
#include <pipeline/pipeline.hpp>
#include <shader/shader.hpp>
#include <swapchain/swapchain.hpp>
#include <texture/texture.hpp>
#include <texture/texture_loader.hpp>
#include <vulkan/vulkan_core.h>
#include <window/window.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

struct Vertex {
    float pos[2];
    float color[3];
    float uv[2];
};

static const std::vector<Vertex> kQuadVertices = {
    { { -0.75f, -0.75f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
    { { 0.75f, -0.75f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
    { { 0.75f, 0.75f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
    { { -0.75f, 0.75f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
};

static const std::vector<uint16_t> kQuadIndices = { 0, 1, 2, 2, 3, 0 };

struct alignas(16) Mat4 {
    float m[16];
};

static Mat4 MakeRotationZ(float radians)
{
    float c = std::cos(radians);
    float s = std::sin(radians);
    return { { c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };
}

int main()
{
    // --- Window + device + swapchain ------------------------------------
    auto winCfg = yst::ywin::CreateConfig(yst::ywinc::DEFAULT_WINDOW);
    winCfg.Width = 1024;
    winCfg.Height = 768;
    winCfg.Title = "YST Textured Quad";
    auto [window, werr] = yst::ywin::CreateWindow(winCfg);
    if (werr) {
        std::cerr << "Window: " << werr.str() << "\n";
        return -1;
    }

    auto deviceCfg = yst::core::CreateConfig(yst::gpuc::DEBUG_CONFIG);
    auto [device, derr] = yst::core::CreateDevice(deviceCfg);
    if (derr) {
        std::cerr << "Device: " << derr.str() << "\n";
        return -1;
    }
    std::cout << "Using GPU: " << device.GetDeviceName() << "\n";

    auto [swapchain, serr] = yst::core::CreateSwapchain(device, window);
    if (serr) {
        std::cerr << "Swapchain: " << serr.str() << "\n";
        return -1;
    }

    // --- Buffers (convenience helpers) ----------------------------------
    auto [vertexBuffer, vbErr] = yst::core::CreateVertexBuffer(
        device, sizeof(Vertex) * kQuadVertices.size());
    if (vbErr) {
        std::cerr << vbErr.str() << "\n";
        return -1;
    }
    if (auto e = vertexBuffer.UploadData(
            kQuadVertices.data(), sizeof(Vertex) * kQuadVertices.size())) {
        std::cerr << e.str() << "\n";
        return -1;
    }

    auto [indexBuffer, ibErr] = yst::core::CreateIndexBuffer(
        device, sizeof(uint16_t) * kQuadIndices.size());
    if (ibErr) {
        std::cerr << ibErr.str() << "\n";
        return -1;
    }
    if (auto e = indexBuffer.UploadData(
            kQuadIndices.data(), sizeof(uint16_t) * kQuadIndices.size())) {
        std::cerr << e.str() << "\n";
        return -1;
    }

    auto [ubo, uboErr] = yst::core::CreateUniformBuffer(device, sizeof(Mat4));
    if (uboErr) {
        std::cerr << uboErr.str() << "\n";
        return -1;
    }

    // --- Shaders --------------------------------------------------------
    auto [vertSpv, ve] = yst::core::LoadSpvFile("./assets/shaders/compiled/vert.spv");
    if (ve) {
        std::cerr << "vert.spv: " << ve.str() << "\n";
        return -1;
    }
    auto [fragSpv, fe] = yst::core::LoadSpvFile("./assets/shaders/compiled/frag.spv");
    if (fe) {
        std::cerr << "frag.spv: " << fe.str() << "\n";
        return -1;
    }

    auto vertCfg = yst::core::ShaderModuleBuilder(yst::core::ShaderModulePreset::Vertex)
                       .WithSpirv(vertSpv)
                       .Build();
    auto [vertMod, vmErr] = yst::core::CreateShaderModule(device, vertCfg);
    if (vmErr) {
        std::cerr << vmErr.str() << "\n";
        return -1;
    }

    auto fragCfg = yst::core::ShaderModuleBuilder(yst::core::ShaderModulePreset::Fragment)
                       .WithSpirv(fragSpv)
                       .Build();
    auto [fragMod, fmErr] = yst::core::CreateShaderModule(device, fragCfg);
    if (fmErr) {
        std::cerr << fmErr.str() << "\n";
        return -1;
    }

    // --- Texture ----------------------------
    uint32_t texW = 0, texH = 0;
    const void* texPixels = nullptr;

    auto [pixels, loadErr] = yst::core::LoadStbImage(
        "./assets/textures/checkerboard.png");
    if (!loadErr) {
        texW = static_cast<uint32_t>(pixels.Width);
        texH = static_cast<uint32_t>(pixels.Height);
        texPixels = pixels.bytes.data();
        std::cout << "Loaded texture: " << texW << "x" << texH << "\n";
    } else {
        std::cerr << "Failed to load image: " << loadErr.str() << "\n";
        return -1;
    }

    auto textureCfg = yst::core::Texture2DBuilder(yst::core::Texture2DPreset::MipmappedRgba8Srgb)
                          .WithPixels(texPixels)
                          .WithExtent(texW, texH)
                          .Build();
    auto [texture, texErr] = yst::core::CreateTexture2D(device, textureCfg);
    if (texErr) {
        std::cerr << texErr.str() << "\n";
        return -1;
    }

    // --- BindGroupLayout via Builder ------------------------------------
    auto bglCfg = yst::core::BindGroupLayoutBuilder(yst::core::BindGroupLayoutPreset::Empty)
                      .AddUniformBuffer(0, yst::core::ShaderStageBits::Vertex)
                      .AddCombinedTextureSampler(1, yst::core::ShaderStageBits::Fragment)
                      .Build();
    auto [bgl, bglErr] = yst::core::CreateBindGroupLayout(device, bglCfg);
    if (bglErr) {
        std::cerr << bglErr.str() << "\n";
        return -1;
    }

    // --- PipelineLayout -------------------------------------------------
    auto pipelineLayoutCfg = yst::core::PipelineLayoutBuilder(yst::core::PipelineLayoutPreset::Empty)
                                 .AddBindGroupLayout(bgl)
                                 .Build();
    auto [pipelineLayout, plErr] = yst::core::CreatePipelineLayout(device, pipelineLayoutCfg);
    if (plErr) {
        std::cerr << plErr.str() << "\n";
        return -1;
    }

    // --- DescriptorPool + BindGroup -------------------------------------
    // Auto-size the pool from the layout before creating it.
    yst::core::DescriptorPoolConfig poolCfg = { .MaxSets = 16 };
    poolCfg.AutoSizeFromLayouts({ &bgl });
    auto [pool, poolErr] = yst::core::CreateDescriptorPool(device, poolCfg);
    if (poolErr) {
        std::cerr << poolErr.str() << "\n";
        return -1;
    }

    yst::core::BindGroupEntry uboEntry = {
        .Binding = 0,
        .Buffer = ubo.buffer,
        .Range = VK_WHOLE_SIZE,
    };

    yst::core::BindGroupEntry textureEntry = {
        .Binding = 1,
        .ImageView = texture.view.view,
        .ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .Sampler = texture.sampler.sampler,
    };

    yst::core::BindGroupConfig bindGroupCfg;
    bindGroupCfg.Layout = &bgl;
    bindGroupCfg.Entries = { uboEntry, textureEntry };
    auto [bg, bgErr] = yst::core::CreateBindGroup(device, pool, bindGroupCfg);
    if (bgErr) {
        std::cerr << bgErr.str() << "\n";
        return -1;
    }

    // --- Graphics pipeline ----------------------------------------------
    auto pipelineCfg = yst::core::PipelineBuilder(yst::core::PipelinePreset::OpaqueGraphics)
                           .WithPipelineLayout(pipelineLayout)
                           .WithRenderPass(swapchain.GetRenderPass())
                           .WithVertexShaderSpv(vertSpv)
                           .WithFragmentShaderSpv(fragSpv)
                           .AddVertexBinding({ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX })
                           .AddVertexAttribute({ 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos) })
                           .AddVertexAttribute({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) })
                           .AddVertexAttribute({ 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) })
                           .Build();
    auto [pipeline, pErr] = yst::core::CreateGraphicsPipeline(device, pipelineCfg);
    if (pErr) {
        std::cerr << pErr.str() << "\n";
        return -1;
    }

    // --- Render loop ----------------------------------------------------
    uint64_t frame = 0;
    while (window.IsOpen()) {
        window.PollEvents();

        // Per-frame: rotate the quad.
        Mat4 mvp = MakeRotationZ(static_cast<float>(frame) * 0.01f);
        if (auto e = ubo.UploadData(&mvp, sizeof(Mat4))) {
            std::cerr << "UBO update: " << e.str() << "\n";
            break;
        }

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
        cmd.BindBindGroup(pipelineLayout.layout, /*setIndex=*/0, bg);
        cmd.BindVertexBuffer(vertexBuffer.buffer);
        cmd.BindIndexBuffer(indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
        cmd.DrawIndexed(kQuadIndices.size());

        cmd.EndRenderPass();
        if (auto pe = swapchain.Present(cmd)) {
            std::cerr << "Present: " << pe.str() << "\n";
            break;
        }
        if (swapchain.NeedsRecreate())
            swapchain.Recreate(window);

        ++frame;
    }

    vkDeviceWaitIdle(device.LogicalDevice);
    return 0;
}

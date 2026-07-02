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
    auto [window, werr] = yst::ywin::CreateWindow({
        .Width = 1024,
        .Height = 768,
        .Title = "YST Textured Quad",
    });
    if (werr) {
        std::cerr << "Window: " << werr.str() << "\n";
        return -1;
    }

    auto [device, derr] = yst::core::CreateDevice({ .EnableDebug = true });
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

    auto [vertMod, vmErr] = yst::core::CreateShaderModule(device, {
                                                                      .Stage = VK_SHADER_STAGE_VERTEX_BIT,
                                                                      .Spirv = vertSpv,
                                                                  });
    if (vmErr) {
        std::cerr << vmErr.str() << "\n";
        return -1;
    }

    auto [fragMod, fmErr] = yst::core::CreateShaderModule(device, {
                                                                      .Stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                      .Spirv = fragSpv,
                                                                  });
    if (fmErr) {
        std::cerr << fmErr.str() << "\n";
        return -1;
    }

    // --- Texture (load PNG; fall back to procedural checkerboard) -------
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

    auto [texture, texErr] = yst::core::CreateTexture2D(device, {
                                                                    .Pixels = texPixels,
                                                                    .Width = texW,
                                                                    .Height = texH,
                                                                    .Channels = 4,
                                                                    .Format = VK_FORMAT_R8G8B8A8_SRGB,
                                                                    .GenerateMipmaps = true,
                                                                    .SamplerCfg = {
                                                                        .AddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                                        .AddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                                    },
                                                                });
    if (texErr) {
        std::cerr << texErr.str() << "\n";
        return -1;
    }

    // --- BindGroupLayout via Builder ------------------------------------
    auto [bgl, bglErr] = yst::core::BindGroupLayoutBuilder()
                             .AddUniformBuffer(0, yst::core::ShaderStageBits::Vertex)
                             .AddCombinedTextureSampler(1, yst::core::ShaderStageBits::Fragment)
                             .Build(device);
    if (bglErr) {
        std::cerr << bglErr.str() << "\n";
        return -1;
    }

    // --- PipelineLayout -------------------------------------------------
    auto [pipelineLayout, plErr] = yst::core::CreatePipelineLayout(device, {
                                                                               .BindGroupLayouts = { &bgl },
                                                                           });
    if (plErr) {
        std::cerr << plErr.str() << "\n";
        return -1;
    }

    // --- DescriptorPool + BindGroup -------------------------------------
    // Auto-size the pool from the layout before creating it.
    yst::core::DescriptorPoolConfig poolCfg {
        .MaxSets = 16,
    };
    poolCfg.AutoSizeFromLayouts({ &bgl });
    auto [pool, poolErr] = yst::core::CreateDescriptorPool(device, poolCfg);
    if (poolErr) {
        std::cerr << poolErr.str() << "\n";
        return -1;
    }

    auto [bg, bgErr] = yst::core::CreateBindGroup(device, pool, {
                                                                    .Layout = &bgl,
                                                                    .Entries = {
                                                                        { .Binding = 0, .Buffer = ubo.buffer, .Range = VK_WHOLE_SIZE },
                                                                        { .Binding = 1, .ImageView = texture.view.view, .Sampler = texture.sampler.sampler, .ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                                                                    },
                                                                });
    if (bgErr) {
        std::cerr << bgErr.str() << "\n";
        return -1;
    }

    // --- Graphics pipeline ----------------------------------------------
    auto [pipeline, pErr] = yst::core::CreateGraphicsPipeline(device, {
                                                                          .PipelineLayoutOverride = &pipelineLayout,
                                                                          .renderPass = swapchain.GetRenderPass(),
                                                                          .vertexShaderSpv = vertSpv,
                                                                          .fragmentShaderSpv = fragSpv,
                                                                          .bindings = { { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX } },
                                                                          .attributes = {
                                                                              { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos) },
                                                                              { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) },
                                                                              { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) },
                                                                          },
                                                                      });
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

    // No explicit Destroy() calls! All resources auto-clean on scope exit.
    // RAII destructors handle: pipeline, pipelineLayout, bgl, pool (frees bg),
    // texture (image+view+sampler), vertMod, fragMod, ubo, indexBuffer,
    // vertexBuffer, swapchain, device, window.
    return 0;
}

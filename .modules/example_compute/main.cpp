#include "errors.hpp"

#include "command/command.hpp"
#include "pipeline/compute_pipeline.hpp"
#include <buffer/buffer.hpp>
#include <descriptor/bind_group.hpp>
#include <descriptor/bind_group_layout.hpp>
#include <descriptor/descriptor_pool.hpp>
#include <descriptor/pipeline_layout.hpp>
#include <device/device.hpp>
#include <errors.hpp>
#include <pipeline/pipeline.hpp>
#include <shader/shader.hpp>
#include <vulkan/vulkan_core.h>

#include <cmath>
#include <cstring>
#include <iostream>

const size_t dataSize = 1024 * sizeof(float);

int main()
{
    auto deviceCfg = yst::core::CreateConfig(yst::gpuc::DEBUG_CONFIG);
    auto [device, derr] = yst::core::CreateDevice(deviceCfg);
    if (derr) {
        std::cerr << "Device: " << derr.str() << "\n";
        return -1;
    }
    std::cout << "Using GPU: " << device.GetDeviceName() << "\n";

    // --- Staging Буферы (in RAM) ---
    // Flags TRANSFER_SRC and TRANSFER_DST are mapped (HostVisible)
    // stagingIn transfers to GPU
    auto [stagingIn, sInErr] = yst::core::CreateStagingBuffer(device, dataSize);

    // stagingOut getting data from GPU
    auto stagingOutCfg = yst::core::BufferBuilder(yst::core::BufferPreset::Staging)
                             .WithSize(dataSize)
                             .AddUsageFlags(VK_BUFFER_USAGE_TRANSFER_DST_BIT)
                             .Build();
    auto [stagingOut, sOutErr] = yst::core::CreateBuffer(device, stagingOutCfg);

    std::vector<float> inputData(1024, 5.0f);
    stagingIn.UploadData(inputData.data(), dataSize);

    // --- GPU Buffers (VRAM, Device Local) ---
    auto gpuInCfg = yst::core::BufferBuilder(yst::core::BufferPreset::Empty)
                        .WithSize(dataSize)
                        .WithUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                            | VK_BUFFER_USAGE_TRANSFER_DST_BIT) // Accepts copying
                        .Build();
    auto [gpuIn, gInErr] = yst::core::CreateBuffer(device, gpuInCfg);

    auto gpuOutCfg = yst::core::BufferBuilder(yst::core::BufferPreset::Empty)
                         .WithSize(dataSize)
                         .WithUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                             | VK_BUFFER_USAGE_TRANSFER_SRC_BIT) // Transfers back
                         .Build();
    auto [gpuOut, gOutErr] = yst::core::CreateBuffer(device, gpuOutCfg);

    // --- BindGroupLayout, PipelineLayout, DescriptorPool, BindGroup ---
    auto bglCfg = yst::core::BindGroupLayoutBuilder(yst::core::BindGroupLayoutPreset::Empty)
                      .AddStorageBuffer(0, true, yst::core::ShaderStageBits::Compute) // ReadOnly = true
                      .AddStorageBuffer(1, false, yst::core::ShaderStageBits::Compute) // ReadOnly = false
                      .Build();
    auto [bgl, bglErr] = yst::core::CreateBindGroupLayout(device, bglCfg);

    auto pipelineLayoutCfg = yst::core::PipelineLayoutBuilder(yst::core::PipelineLayoutPreset::Empty)
                                 .AddBindGroupLayout(bgl)
                                 .Build();
    auto [pipelineLayout, plErr] = yst::core::CreatePipelineLayout(device, pipelineLayoutCfg);

    yst::core::DescriptorPoolConfig poolCfg = { .MaxSets = 1 };
    poolCfg.AutoSizeFromLayouts({ &bgl });
    auto [pool, poolErr] = yst::core::CreateDescriptorPool(device, poolCfg);

    yst::core::BindGroupConfig bindGroupCfg;
    bindGroupCfg.Layout = &bgl;
    bindGroupCfg.Entries = {
        { .Binding = 0, .Buffer = gpuIn.buffer, .Range = VK_WHOLE_SIZE },
        { .Binding = 1, .Buffer = gpuOut.buffer, .Range = VK_WHOLE_SIZE }
    };
    auto [bg, bgErr] = yst::core::CreateBindGroup(device, pool, bindGroupCfg);

    // --- Compute Pipeline ---
    auto [computeSpv, spvErr] = yst::core::LoadSpvFile("./assets/shaders/compiled/compute.spv");
    auto computePipelineCfg = yst::core::ComputePipelineBuilder()
                                  .WithPipelineLayout(pipelineLayout.layout)
                                  .WithShaderSpv(computeSpv)
                                  .Build();
    auto [computePipeline, cpErr] = yst::core::CreateComputePipeline(device, computePipelineCfg);

    auto submitErr = yst::core::SubmitOneTimeCommands(device,
        [&](yst::core::CommandList& cmd) -> yst::CustomError {
            cmd.Begin();

            // 5.1. Copying RAM (stagingIn) into VRAM (gpuIn)
            cmd.CopyBuffer(stagingIn.buffer, gpuIn.buffer, dataSize);

            // 5.2. Barrier
            // RANSFER_WRITE -> SHADER_READ
            cmd.PipelineBarrierBuffer(gpuIn.buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT);

            // 5.3. Computing
            cmd.BindPipeline(computePipeline.pipeline, yst::core::PipelineBindPoint::Compute);
            cmd.BindBindGroup(pipelineLayout.layout, 0, bg, yst::core::PipelineBindPoint::Compute);
            cmd.Dispatch(16, 1, 1); // 1024 / 64 = 16 groups

            // 5.4. Barrier SHADER_WRITE -> TRANSFER_READ
            cmd.PipelineBarrierBuffer(gpuOut.buffer,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT);

            // 5.5 Copying from VRAM (gpuOut) to RAM (stagingOut)
            cmd.CopyBuffer(gpuOut.buffer, stagingOut.buffer, dataSize);

            cmd.End();
            return yst::CustomError();
        });

    // --- reading data to CPU ---
    float* result = static_cast<float*>(stagingOut.MappedData());
    std::cout << "Compute result [0] (via Staging): " << result[0] << "\n";

    vkDeviceWaitIdle(device.LogicalDevice);
    return 0;
}

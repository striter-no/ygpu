#include <pipeline/pipeline.hpp>

#include <array>

namespace yst::core {

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept
{
    device_ = other.device_;
    ownsLayout_ = other.ownsLayout_;
    pipeline = other.pipeline;
    layout = other.layout;

    other.device_ = nullptr;
    other.ownsLayout_ = false;
    other.pipeline = VK_NULL_HANDLE;
    other.layout = VK_NULL_HANDLE;
}

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& other) noexcept
{
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        ownsLayout_ = other.ownsLayout_;
        pipeline = other.pipeline;
        layout = other.layout;

        other.device_ = nullptr;
        other.ownsLayout_ = false;
        other.pipeline = VK_NULL_HANDLE;
        other.layout = VK_NULL_HANDLE;
    }
    return *this;
}

void GraphicsPipeline::Destroy()
{
    if (!device_)
        return;
    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_->LogicalDevice, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (ownsLayout_ && layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_->LogicalDevice, layout, nullptr);
        layout = VK_NULL_HANDLE;
    }
    ownsLayout_ = false;
    device_ = nullptr;
}

namespace {

    /// Build a VkShaderModule from SPIR-V bytecode. Returns VK_NULL_HANDLE on
    /// failure and the caller surfaces an error via CustomError.
    VkResult BuildShaderModule(Device& device, const std::vector<uint32_t>& spv,
        VkShaderModule* outModule)
    {
        if (spv.empty()) {
            *outModule = VK_NULL_HANDLE;
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        VkShaderModuleCreateInfo info {};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = spv.size() * sizeof(uint32_t);
        info.pCode = spv.data();
        return vkCreateShaderModule(device.LogicalDevice, &info, nullptr, outModule);
    }

    CustomError BuildShaderStage(Device& device,
        const std::vector<uint32_t>& spv,
        VkShaderStageFlagBits stage,
        const std::string& entryPoint,
        VkPipelineShaderStageCreateInfo& outStage,
        VkShaderModule& outModule)
    {
        outModule = VK_NULL_HANDLE;
        if (spv.empty()) {
            // Stage simply not present; caller decides whether that's an error.
            outStage = {};
            return CustomError();
        }

        if (BuildShaderModule(device, spv, &outModule) != VK_SUCCESS) {
            return CustomError(ErrorCode::ShaderModuleCreationFailed,
                "Failed to create shader module for stage "
                    + std::to_string(stage));
        }

        outStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        outStage.stage = stage;
        outStage.module = outModule;
        outStage.pName = entryPoint.c_str();
        outStage.pSpecializationInfo = nullptr;
        return CustomError();
    }

} // namespace

std::pair<GraphicsPipeline, CustomError> CreateGraphicsPipeline(
    Device& device, const PipelineConfig& config)
{
    GraphicsPipeline out;
    out.device_ = &device;

    // 1. Shader stages.
    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(2 + config.ExtraShaderStages.size());

    VkPipelineShaderStageCreateInfo vertStage {};
    if (auto err = BuildShaderStage(device, config.vertexShaderSpv,
            VK_SHADER_STAGE_VERTEX_BIT, config.VertexShaderEntryPoint,
            vertStage, vertModule)) {
        return { std::move(out), err };
    }
    if (vertStage.module)
        shaderStages.push_back(vertStage);

    VkPipelineShaderStageCreateInfo fragStage {};
    if (auto err = BuildShaderStage(device, config.fragmentShaderSpv,
            VK_SHADER_STAGE_FRAGMENT_BIT, config.FragmentShaderEntryPoint,
            fragStage, fragModule)) {
        if (vertModule)
            vkDestroyShaderModule(device.LogicalDevice, vertModule, nullptr);
        return { std::move(out), err };
    }
    if (fragStage.module)
        shaderStages.push_back(fragStage);

    // Caller-supplied extra stages (geometry, tessellation, task/mesh, ...).
    for (const auto& s : config.ExtraShaderStages)
        shaderStages.push_back(s);

    // 2. Vertex input.
    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(config.bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = config.bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(config.attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = config.attributes.data();

    // 3. Input assembly.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = config.Topology;
    inputAssembly.primitiveRestartEnable = config.PrimitiveRestartEnable;

    // 4. Tessellation (only if patch control points > 0).
    VkPipelineTessellationStateCreateInfo tessellationState {};
    tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    const VkPipelineTessellationStateCreateInfo* tessellationPtr = nullptr;
    if (config.PatchControlPoints > 0) {
        tessellationState.patchControlPoints = config.PatchControlPoints;
        tessellationPtr = &tessellationState;
    }

    // 5. Viewport state (always 1 viewport / 1 scissor; the dynamic state
    //    below makes the actual extent set at draw time).
    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 6. Rasterizer.
    VkPipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = config.DepthClampEnable;
    rasterizer.rasterizerDiscardEnable = config.RasterizerDiscardEnable;
    rasterizer.polygonMode = config.PolygonMode;
    rasterizer.lineWidth = config.LineWidth;
    rasterizer.cullMode = config.CullMode;
    rasterizer.frontFace = config.FrontFace;

    // 7. Multisample.
    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = config.SampleShadingEnable;
    multisampling.minSampleShading = config.MinSampleShading;
    multisampling.rasterizationSamples = config.RasterizationSamples;
    multisampling.pSampleMask = config.SampleMask;
    multisampling.alphaToCoverageEnable = config.AlphaToCoverageEnable;
    multisampling.alphaToOneEnable = config.AlphaToOneEnable;

    // 8. Color blend. Synthesise a default opaque attachment when the caller
    //    didn't supply any — this matches the historical behaviour.
    VkPipelineColorBlendAttachmentState defaultAttachment {};
    defaultAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    defaultAttachment.blendEnable = VK_FALSE;

    const std::vector<VkPipelineColorBlendAttachmentState>* blendAttachmentsPtr = &config.ColorBlendAttachments;
    if (blendAttachmentsPtr->empty()) {
        static const std::vector<VkPipelineColorBlendAttachmentState> fallback {
            defaultAttachment
        };
        blendAttachmentsPtr = &fallback;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = config.LogicOpEnable;
    colorBlending.logicOp = config.LogicOp;
    colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachmentsPtr->size());
    colorBlending.pAttachments = blendAttachmentsPtr->data();
    for (int i = 0; i < 4; ++i)
        colorBlending.blendConstants[i] = config.BlendConstants[i];

    // 9. Dynamic state. Defaults to {VIEWPORT, SCISSOR} for backward compat.
    std::vector<VkDynamicState> dynStates = config.DynamicStates;
    if (dynStates.empty()) {
        dynStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    }
    VkPipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dynamicState.pDynamicStates = dynStates.data();

    // 10. Pipeline layout. If the caller supplied a pre-built
    //     PipelineLayout (preferred path when using the descriptor module),
    //     use it directly. Otherwise build one from the raw
    //     DescriptorSetLayouts + PushConstantRanges for backward
    //     compatibility with the old API.
    if (config.PipelineLayoutOverride
        && config.PipelineLayoutOverride->layout != VK_NULL_HANDLE) {
        // Use the external layout but mark that we don't own it — the
        // caller is responsible for keeping the PipelineLayout alive for
        // the pipeline's lifetime and destroying it separately.
        const VkPipelineLayout externalLayout = config.PipelineLayoutOverride->layout;
        out.ownsLayout_ = false;
        // out.layout stays VK_NULL_HANDLE — Destroy() won't touch the
        // external layout. The pipeline itself uses externalLayout for
        // creation only.

        VkGraphicsPipelineCreateInfo pipelineInfo {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pTessellationState = tessellationPtr;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = config.DepthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = externalLayout;
        pipelineInfo.renderPass = config.renderPass;
        pipelineInfo.subpass = config.Subpass;
        pipelineInfo.basePipelineHandle = config.BasePipelineHandle;
        pipelineInfo.basePipelineIndex = config.BasePipelineIndex;

        VkResult res = vkCreateGraphicsPipelines(device.LogicalDevice,
            config.PipelineCache, 1, &pipelineInfo, nullptr, &out.pipeline);

        if (vertModule)
            vkDestroyShaderModule(device.LogicalDevice, vertModule, nullptr);
        if (fragModule)
            vkDestroyShaderModule(device.LogicalDevice, fragModule, nullptr);

        if (res != VK_SUCCESS) {
            return { std::move(out),
                CustomError(ErrorCode::GraphicsPipelineCreationFailed,
                    "Failed to create graphics pipeline") };
        }

        return { std::move(out), CustomError() };
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(config.DescriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = config.DescriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(config.PushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = config.PushConstantRanges.data();

    if (vkCreatePipelineLayout(device.LogicalDevice, &pipelineLayoutInfo,
            nullptr, &out.layout)
        != VK_SUCCESS) {
        if (vertModule)
            vkDestroyShaderModule(device.LogicalDevice, vertModule, nullptr);
        if (fragModule)
            vkDestroyShaderModule(device.LogicalDevice, fragModule, nullptr);
        return { std::move(out),
            CustomError(ErrorCode::PipelineLayoutCreationFailed,
                "Failed to create pipeline layout") };
    }
    out.ownsLayout_ = true;

    // 11. Graphics pipeline create info.
    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState = tessellationPtr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = config.DepthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = out.layout;
    pipelineInfo.renderPass = config.renderPass;
    pipelineInfo.subpass = config.Subpass;
    pipelineInfo.basePipelineHandle = config.BasePipelineHandle;
    pipelineInfo.basePipelineIndex = config.BasePipelineIndex;

    VkResult res = vkCreateGraphicsPipelines(device.LogicalDevice,
        config.PipelineCache, 1, &pipelineInfo, nullptr, &out.pipeline);

    // Shader modules can be destroyed immediately after pipeline creation.
    if (vertModule)
        vkDestroyShaderModule(device.LogicalDevice, vertModule, nullptr);
    if (fragModule)
        vkDestroyShaderModule(device.LogicalDevice, fragModule, nullptr);

    if (res != VK_SUCCESS) {
        vkDestroyPipelineLayout(device.LogicalDevice, out.layout, nullptr);
        out.layout = VK_NULL_HANDLE;
        return { std::move(out),
            CustomError(ErrorCode::GraphicsPipelineCreationFailed,
                "Failed to create graphics pipeline") };
    }

    return { std::move(out), CustomError() };
}

} // namespace yst::core

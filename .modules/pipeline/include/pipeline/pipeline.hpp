#pragma once
#include <vulkan/vulkan.h>

#include <device/device.hpp>
#include <string>
#include <vector>

namespace yst::core {

struct GraphicsPipeline {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    void Destroy(Device& device);
};

/// Layer 2 configuration for a graphics pipeline.
///
/// Every field defaults to the value that the historical hardcoded pipeline
/// code used to set, so callers that construct `PipelineConfig{}` get the
/// exact same pipeline as before. Advanced callers can override any subset
/// of fields.
struct PipelineConfig {
    // ---- Vertex input -------------------------------------------------
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    // ---- Input assembly ----------------------------------------------
    VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkBool32 PrimitiveRestartEnable = VK_FALSE;

    // ---- Tessellation (optional) -------------------------------------
    uint32_t PatchControlPoints = 0; ///< 0 = no tessellation stage.

    // ---- Rasterizer ---------------------------------------------------
    VkBool32 DepthClampEnable = VK_FALSE;
    VkBool32 RasterizerDiscardEnable = VK_FALSE;
    VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
    VkFrontFace FrontFace = VK_FRONT_FACE_CLOCKWISE;
    float LineWidth = 1.0f;

    // ---- Multisample --------------------------------------------------
    VkSampleCountFlagBits RasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkBool32 SampleShadingEnable = VK_FALSE;
    float MinSampleShading = 0.0f;
    const VkSampleMask* SampleMask = nullptr;
    VkBool32 AlphaToCoverageEnable = VK_FALSE;
    VkBool32 AlphaToOneEnable = VK_FALSE;

    // ---- Depth/stencil (optional) ------------------------------------
    /// If non-null, depth/stencil testing is enabled with the given state.
    /// If null, no VkPipelineDepthStencilStateCreateInfo is chained.
    const VkPipelineDepthStencilStateCreateInfo* DepthStencil = nullptr;

    // ---- Color blend --------------------------------------------------
    /// Per-attachment blend states. If empty, a single backwards-compatible
    /// attachment is synthesized with blendEnable=VK_FALSE and full RGBA
    /// write mask.
    std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachments;
    VkBool32 LogicOpEnable = VK_FALSE;
    VkLogicOp LogicOp = VK_LOGIC_OP_COPY;
    float BlendConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // ---- Dynamic state ------------------------------------------------
    /// If empty, defaults to {VIEWPORT, SCISSOR}.
    std::vector<VkDynamicState> DynamicStates;

    // ---- Shader stages ------------------------------------------------
    std::vector<uint32_t> vertexShaderSpv;
    std::vector<uint32_t> fragmentShaderSpv;
    /// Optional: additional stages (geometry, tess control/eval, compute
    /// shaders used as graphics stages, task/mesh). The vertex and fragment
    /// modules above are always added; this vector is for everything else.
    std::vector<VkPipelineShaderStageCreateInfo> ExtraShaderStages;
    std::string VertexShaderEntryPoint = "main";
    std::string FragmentShaderEntryPoint = "main";

    // ---- Layout / render pass integration ----------------------------
    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    std::vector<VkPushConstantRange> PushConstantRanges;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t Subpass = 0;

    /// Pipeline cache to accelerate creation. Optional.
    VkPipelineCache PipelineCache = VK_NULL_HANDLE;

    /// Optional base pipeline handle + index for derived pipelines.
    VkPipeline BasePipelineHandle = VK_NULL_HANDLE;
    int32_t BasePipelineIndex = -1;

    /// Helper: push a standard "no blend, write RGBA" attachment state.
    /// Useful when ColorBlendAttachments should be empty by default but the
    /// caller wants to explicitly opt into a known attachment count.
    PipelineConfig& AddOpaqueColorAttachment() noexcept
    {
        VkPipelineColorBlendAttachmentState att {};
        att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        att.blendEnable = VK_FALSE;
        ColorBlendAttachments.push_back(att);
        return *this;
    }
};

std::pair<GraphicsPipeline, CustomError> CreateGraphicsPipeline(
    Device& device, const PipelineConfig& config);

} // namespace yst::core

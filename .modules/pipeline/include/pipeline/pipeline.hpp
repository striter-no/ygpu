#pragma once
#include <vulkan/vulkan.h>

#include <descriptor/pipeline_layout.hpp>
#include <device/device.hpp>
#include <string>
#include <vector>

namespace yst::core {

// Forward declaration so the friend declaration inside GraphicsPipeline
// (which references CreateGraphicsPipeline's signature) can name the type.
struct PipelineConfig;

struct GraphicsPipeline {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    GraphicsPipeline() = default;
    ~GraphicsPipeline() { Destroy(); }

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

    void Destroy();

private:
    Device* device_ = nullptr;
    bool ownsLayout_ = false;

    friend std::pair<GraphicsPipeline, CustomError> CreateGraphicsPipeline(
        Device& device, const PipelineConfig& config);
};

struct PipelineConfig {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkBool32 PrimitiveRestartEnable = VK_FALSE;

    uint32_t PatchControlPoints = 0;

    VkBool32 DepthClampEnable = VK_FALSE;
    VkBool32 RasterizerDiscardEnable = VK_FALSE;
    VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
    VkFrontFace FrontFace = VK_FRONT_FACE_CLOCKWISE;
    float LineWidth = 1.0f;

    VkSampleCountFlagBits RasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkBool32 SampleShadingEnable = VK_FALSE;
    float MinSampleShading = 0.0f;
    const VkSampleMask* SampleMask = nullptr;
    VkBool32 AlphaToCoverageEnable = VK_FALSE;
    VkBool32 AlphaToOneEnable = VK_FALSE;

    const VkPipelineDepthStencilStateCreateInfo* DepthStencil = nullptr;

    std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachments;
    VkBool32 LogicOpEnable = VK_FALSE;
    VkLogicOp LogicOp = VK_LOGIC_OP_COPY;
    float BlendConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    std::vector<VkDynamicState> DynamicStates;

    std::vector<uint32_t> vertexShaderSpv;
    std::vector<uint32_t> fragmentShaderSpv;
    std::vector<VkPipelineShaderStageCreateInfo> ExtraShaderStages;
    std::string VertexShaderEntryPoint = "main";
    std::string FragmentShaderEntryPoint = "main";

    PipelineLayout* PipelineLayoutOverride = nullptr;
    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    std::vector<VkPushConstantRange> PushConstantRanges;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t Subpass = 0;

    VkPipelineCache PipelineCache = VK_NULL_HANDLE;
    VkPipeline BasePipelineHandle = VK_NULL_HANDLE;
    int32_t BasePipelineIndex = -1;

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

#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <device/device.hpp>
#include <errors.hpp>

#include "config.hpp"

namespace yst::core {

struct BindGroupLayoutConfig {
    std::vector<BindingLayoutEntry> Entries;
    VkDescriptorSetLayoutCreateFlags Flags = 0;
};

/// Owning wrapper around a VkDescriptorSetLayout. RAII.
class BindGroupLayout {
public:
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    BindGroupLayoutConfig config;

    BindGroupLayout() = default;
    ~BindGroupLayout() { Destroy(); }

    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;
    BindGroupLayout(BindGroupLayout&& other) noexcept;
    BindGroupLayout& operator=(BindGroupLayout&& other) noexcept;

    void Destroy();

    void AccumulatePoolSizes(std::vector<VkDescriptorPoolSize>& sizes,
        uint32_t setCount) const;

private:
    Device* device_ = nullptr;

    friend std::pair<BindGroupLayout, CustomError> CreateBindGroupLayout(
        Device& device, const BindGroupLayoutConfig& config);
    friend class BindGroupLayoutBuilder;
};

std::pair<BindGroupLayout, CustomError> CreateBindGroupLayout(
    Device& device, const BindGroupLayoutConfig& config);

/// Fluent builder for BindGroupLayout. Mirrors wgpu::BindGroupLayoutDescriptor
/// but with a chainable API so callers don't have to construct temporary
/// BindingLayoutEntry objects by hand.
///
/// Usage:
///   auto [bgl, err] = BindGroupLayoutBuilder()
///       .AddUniformBuffer(0, ShaderStageBits::Vertex)
///       .AddCombinedTextureSampler(1, ShaderStageBits::Fragment)
///       .Build(device);
class BindGroupLayoutBuilder {
public:
    BindGroupLayoutBuilder& AddUniformBuffer(uint32_t binding,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics)
    {
        BindingLayoutEntry e;
        e.AsUniformBuffer(binding, visibility);
        config_.Entries.push_back(e);
        return *this;
    }

    BindGroupLayoutBuilder& AddStorageBuffer(uint32_t binding,
        bool readOnly = false,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics)
    {
        BindingLayoutEntry e;
        e.AsStorageBuffer(binding, readOnly, visibility);
        config_.Entries.push_back(e);
        return *this;
    }

    BindGroupLayoutBuilder& AddSampler(uint32_t binding,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics)
    {
        BindingLayoutEntry e;
        e.AsSampler(binding, visibility);
        config_.Entries.push_back(e);
        return *this;
    }

    BindGroupLayoutBuilder& AddSampledTexture(uint32_t binding,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics)
    {
        BindingLayoutEntry e;
        e.AsSampledTexture(binding, visibility);
        config_.Entries.push_back(e);
        return *this;
    }

    BindGroupLayoutBuilder& AddCombinedTextureSampler(uint32_t binding,
        VkShaderStageFlags visibility = ShaderStageBits::AllGraphics)
    {
        BindingLayoutEntry e;
        e.AsCombinedTextureSampler(binding, visibility);
        config_.Entries.push_back(e);
        return *this;
    }

    BindGroupLayoutBuilder& AddEntry(const BindingLayoutEntry& entry)
    {
        config_.Entries.push_back(entry);
        return *this;
    }

    BindGroupLayoutBuilder& WithFlags(VkDescriptorSetLayoutCreateFlags flags)
    {
        config_.Flags = flags;
        return *this;
    }

    std::pair<BindGroupLayout, CustomError> Build(Device& device)
    {
        return CreateBindGroupLayout(device, config_);
    }

    const BindGroupLayoutConfig& Config() const { return config_; }

private:
    BindGroupLayoutConfig config_;
};

} // namespace yst::core

#include <descriptor/bind_group_layout.hpp>

#include <algorithm>
#include <unordered_map>

namespace yst::core {

VkDescriptorType ToVkDescriptorType(BindingType type) noexcept
{
    switch (type) {
    case BindingType::UniformBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case BindingType::StorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case BindingType::ReadonlyStorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case BindingType::SampledTexture:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case BindingType::StorageTexture:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case BindingType::Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case BindingType::CombinedTextureSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

BindGroupLayout::BindGroupLayout(BindGroupLayout&& other) noexcept
{
    device_ = other.device_;
    layout = other.layout;
    config = std::move(other.config);
    other.device_ = nullptr;
    other.layout = VK_NULL_HANDLE;
}

BindGroupLayout& BindGroupLayout::operator=(BindGroupLayout&& other) noexcept
{
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        layout = other.layout;
        config = std::move(other.config);
        other.device_ = nullptr;
        other.layout = VK_NULL_HANDLE;
    }
    return *this;
}

void BindGroupLayout::Destroy()
{
    if (layout != VK_NULL_HANDLE && device_) {
        vkDestroyDescriptorSetLayout(device_->LogicalDevice, layout, nullptr);
        layout = VK_NULL_HANDLE;
        device_ = nullptr;
    }
}

void BindGroupLayout::AccumulatePoolSizes(
    std::vector<VkDescriptorPoolSize>& sizes, uint32_t setCount) const
{
    std::unordered_map<VkDescriptorType, uint32_t> byType;
    for (const auto& e : config.Entries) {
        byType[ToVkDescriptorType(e.Type)] += e.Count * setCount;
    }
    for (const auto& [type, count] : byType) {
        auto it = std::find_if(sizes.begin(), sizes.end(),
            [&](const VkDescriptorPoolSize& s) { return s.type == type; });
        if (it == sizes.end()) {
            sizes.push_back({ type, count });
        } else {
            it->descriptorCount += count;
        }
    }
}

std::pair<BindGroupLayout, CustomError> CreateBindGroupLayout(
    Device& device, const BindGroupLayoutConfig& config)
{
    BindGroupLayout out;
    out.device_ = &device;
    out.config = config;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(config.Entries.size());
    for (const auto& e : config.Entries) {
        VkDescriptorSetLayoutBinding b {};
        b.binding = e.Binding;
        b.descriptorType = ToVkDescriptorType(e.Type);
        b.descriptorCount = e.Count;
        b.stageFlags = e.Visibility;
        b.pImmutableSamplers = nullptr;
        bindings.push_back(b);
    }

    VkDescriptorSetLayoutCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.flags = config.Flags;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device.LogicalDevice, &info, nullptr,
            &out.layout)
        != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::DescriptorSetLayoutCreationFailed,
                "Failed to create BindGroupLayout") };
    }

    return { std::move(out), CustomError() };
}

} // namespace yst::core

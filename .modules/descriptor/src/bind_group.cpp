#include <descriptor/bind_group.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace yst::core {

BindGroupConfig CreateConfig(BindGroupPreset preset)
{
    BindGroupConfig cfg;
    switch (preset) {
    case BindGroupPreset::Empty:
    default:
        return cfg;
    }
}

CustomError BindGroup::Free(Device& device, DescriptorPool& pool)
{
    if (set == VK_NULL_HANDLE || pool.pool == VK_NULL_HANDLE)
        return CustomError();

    vkFreeDescriptorSets(device.LogicalDevice, pool.pool, 1, &set);
    set = VK_NULL_HANDLE;
    layout = nullptr;
    return CustomError();
}

CustomError BindGroup::Update(Device& device, const BindGroupConfig& config)
{
    if (set == VK_NULL_HANDLE) {
        return CustomError(ErrorCode::DescriptorSetUpdateFailed,
            "Cannot update a null BindGroup");
    }
    if (!config.Layout) {
        return CustomError(ErrorCode::DescriptorSetUpdateFailed,
            "BindGroupConfig::Layout must be set");
    }
    if (config.Layout->layout != layout->layout) {
        return CustomError(ErrorCode::DescriptorSetUpdateFailed,
            "BindGroupConfig::Layout does not match the BindGroup's layout");
    }

    // VkWriteDescriptorSet holds RAW pointers to VkDescriptorBufferInfo /
    // VkDescriptorImageInfo. Those pointers must remain valid for the
    // duration of the vkUpdateDescriptorSets call.
    //
    // We store the info structs in two stable vectors (pre-reserved so they
    // don't reallocate during the loop, which would invalidate pointers
    // we've already stored in the writes). Each write points to the
    // corresponding element of one of these vectors.
    //
    // NOTE: a previous version of this code embedded the info structs inside
    // a ResolvedWrite helper struct returned from a free function. The
    // helper's local storage was destroyed on return, leaving dangling
    // pointers in the writes — which manifested as "buffer is VK_NULL_HANDLE"
    // validation errors (the freed memory happened to be zeroed).
    const size_t n = config.Entries.size();
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet> writes;
    bufferInfos.reserve(n);
    imageInfos.reserve(n);
    writes.reserve(n);

    for (const auto& e : config.Entries) {
        // Find the layout entry for this binding index.
        const BindingLayoutEntry* le = nullptr;
        for (const auto& l : config.Layout->config.Entries) {
            if (l.Binding == e.Binding) {
                le = &l;
                break;
            }
        }
        if (!le) {
            return CustomError(ErrorCode::DescriptorSetUpdateFailed,
                "Binding " + std::to_string(e.Binding)
                    + " does not match the layout");
        }

        VkWriteDescriptorSet write {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = e.Binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = ToVkDescriptorType(le->Type);

        switch (le->Type) {
        case BindingType::UniformBuffer:
        case BindingType::StorageBuffer:
        case BindingType::ReadonlyStorageBuffer: {
            if (e.Buffer == VK_NULL_HANDLE) {
                return CustomError(ErrorCode::DescriptorSetUpdateFailed,
                    "Binding " + std::to_string(e.Binding)
                        + " (buffer) has null Buffer");
            }
            VkDescriptorBufferInfo bi {};
            bi.buffer = e.Buffer;
            bi.offset = e.Offset;
            bi.range = e.Range;
            bufferInfos.push_back(bi);
            // Safe: reserve(n) guarantees no reallocation, so the address
            // of the just-pushed element remains valid through the
            // vkUpdateDescriptorSets call below.
            write.pBufferInfo = &bufferInfos.back();
            break;
        }

        case BindingType::SampledTexture:
        case BindingType::StorageTexture: {
            if (e.ImageView == VK_NULL_HANDLE) {
                return CustomError(ErrorCode::DescriptorSetUpdateFailed,
                    "Binding " + std::to_string(e.Binding)
                        + " (sampled/storage texture) has null ImageView");
            }
            VkDescriptorImageInfo ii {};
            ii.sampler = VK_NULL_HANDLE;
            ii.imageView = e.ImageView;
            ii.imageLayout = e.ImageLayout;
            imageInfos.push_back(ii);
            write.pImageInfo = &imageInfos.back();
            break;
        }

        case BindingType::Sampler: {
            if (e.Sampler == VK_NULL_HANDLE) {
                return CustomError(ErrorCode::DescriptorSetUpdateFailed,
                    "Binding " + std::to_string(e.Binding)
                        + " (sampler) has null Sampler");
            }
            VkDescriptorImageInfo ii {};
            ii.sampler = e.Sampler;
            ii.imageView = VK_NULL_HANDLE;
            ii.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfos.push_back(ii);
            write.pImageInfo = &imageInfos.back();
            break;
        }

        case BindingType::CombinedTextureSampler: {
            if (e.ImageView == VK_NULL_HANDLE || e.Sampler == VK_NULL_HANDLE) {
                return CustomError(ErrorCode::DescriptorSetUpdateFailed,
                    "Binding " + std::to_string(e.Binding)
                        + " (combined texture+sampler) has null ImageView or Sampler");
            }
            VkDescriptorImageInfo ii {};
            ii.sampler = e.Sampler;
            ii.imageView = e.ImageView;
            ii.imageLayout = e.ImageLayout;
            imageInfos.push_back(ii);
            write.pImageInfo = &imageInfos.back();
            break;
        }
        }

        writes.push_back(write);
        // writes.push_back may reallocate the writes vector, but that's
        // fine — VkWriteDescriptorSet is a value type and its pBufferInfo
        // / pImageInfo pointers point into bufferInfos / imageInfos, which
        // are separate vectors and not affected by writes' reallocation.
    }

    if (!writes.empty()) {
        vkUpdateDescriptorSets(device.LogicalDevice,
            static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    return CustomError();
}

std::pair<BindGroup, CustomError> AllocateBindGroup(
    Device& device, DescriptorPool& pool, BindGroupLayout& layout)
{
    BindGroup out;
    out.layout = &layout;

    if (pool.pool == VK_NULL_HANDLE) {
        return { std::move(out),
            CustomError(ErrorCode::DescriptorPoolExhausted,
                "DescriptorPool is null") };
    }
    if (layout.layout == VK_NULL_HANDLE) {
        return { std::move(out),
            CustomError(ErrorCode::DescriptorSetLayoutCreationFailed,
                "BindGroupLayout is null") };
    }

    VkDescriptorSetAllocateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = pool.pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout.layout;

    const VkResult res = vkAllocateDescriptorSets(
        device.LogicalDevice, &info, &out.set);

    if (res == VK_ERROR_OUT_OF_POOL_MEMORY || res == VK_ERROR_FRAGMENTED_POOL) {
        return { std::move(out),
            CustomError(ErrorCode::DescriptorPoolExhausted,
                "Descriptor pool exhausted or fragmented") };
    }
    if (res != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::DescriptorSetAllocationFailed,
                "Failed to allocate descriptor set") };
    }

    return { std::move(out), CustomError() };
}

std::pair<BindGroup, CustomError> CreateBindGroup(
    Device& device, DescriptorPool& pool, const BindGroupConfig& config)
{
    if (!config.Layout) {
        return { BindGroup {},
            CustomError(ErrorCode::DescriptorSetUpdateFailed,
                "BindGroupConfig::Layout must be set") };
    }

    auto [bg, allocErr] = AllocateBindGroup(device, pool, *config.Layout);
    if (allocErr)
        return { std::move(bg), allocErr };

    if (auto updateErr = bg.Update(device, config)) {
        bg.Free(device, pool);
        return { std::move(bg), updateErr };
    }

    return { std::move(bg), CustomError() };
}

} // namespace yst::core


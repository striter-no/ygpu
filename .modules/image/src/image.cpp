// yst image module implementation — Builder pattern (v3)
#include <image/image.hpp>

namespace yst::core {

ImageConfig CreateConfig(ImagePreset preset)
{
    ImageConfig cfg;

    switch (preset) {
    case ImagePreset::DepthAttachment:
        cfg.Type = VK_IMAGE_TYPE_2D;
        cfg.Format = VK_FORMAT_D32_SFLOAT;
        cfg.Extent = { 1, 1, 1 };
        cfg.MipLevels = 1;
        cfg.ArrayLayers = 1;
        cfg.Samples = VK_SAMPLE_COUNT_1_BIT;
        cfg.Tiling = VK_IMAGE_TILING_OPTIMAL;
        cfg.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        cfg.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return cfg;

    case ImagePreset::Texture2D:
    default:
        cfg.Type = VK_IMAGE_TYPE_2D;
        cfg.Format = VK_FORMAT_R8G8B8A8_SRGB;
        cfg.Extent = { 1, 1, 1 };
        cfg.MipLevels = 1;
        cfg.ArrayLayers = 1;
        cfg.Samples = VK_SAMPLE_COUNT_1_BIT;
        cfg.Tiling = VK_IMAGE_TILING_OPTIMAL;
        cfg.Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        cfg.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return cfg;
    }
}

ImageViewConfig CreateConfig(ImageViewPreset preset)
{
    ImageViewConfig cfg;
    switch (preset) {
    case ImageViewPreset::Depth2D:
        cfg.Type = VK_IMAGE_VIEW_TYPE_2D;
        cfg.AspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        return cfg;

    case ImageViewPreset::Color2D:
    default:
        cfg.Type = VK_IMAGE_VIEW_TYPE_2D;
        cfg.AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        return cfg;
    }
}

// ============================================================================
// Image
// ============================================================================

Image::Image(Image&& other) noexcept
{
    device_ = other.device_;
    image = other.image;
    allocation_ = other.allocation_;
    format = other.format;
    extent = other.extent;
    mipLevels = other.mipLevels;
    arrayLayers = other.arrayLayers;

    other.device_ = nullptr;
    other.image = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.extent = { 0, 0, 0 };
    other.mipLevels = 1;
    other.arrayLayers = 1;
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        image = other.image;
        allocation_ = other.allocation_;
        format = other.format;
        extent = other.extent;
        mipLevels = other.mipLevels;
        arrayLayers = other.arrayLayers;

        other.device_ = nullptr;
        other.image = VK_NULL_HANDLE;
        other.allocation_ = VK_NULL_HANDLE;
        other.extent = { 0, 0, 0 };
        other.mipLevels = 1;
        other.arrayLayers = 1;
    }
    return *this;
}

void Image::Destroy()
{
    if (image != VK_NULL_HANDLE && device_) {
        vmaDestroyImage(device_->Allocator, image, allocation_);
        image = VK_NULL_HANDLE;
        allocation_ = VK_NULL_HANDLE;
        device_ = nullptr;
    }
}

std::pair<Image, CustomError> CreateImage(
    Device& device, const ImageConfig& config)
{
    Image out;
    out.device_ = &device;
    out.format = config.Format;
    out.extent = config.Extent;
    out.mipLevels = config.MipLevels;
    out.arrayLayers = config.ArrayLayers;

    if (config.Extent.width == 0 || config.Extent.height == 0
        || config.Extent.depth == 0) {
        return { std::move(out),
            CustomError(ErrorCode::ImageCreationFailed,
                "Cannot create an image with a zero-sized extent") };
    }

    VkImageCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = config.Type;
    info.format = config.Format;
    info.extent = config.Extent;
    info.mipLevels = config.MipLevels;
    info.arrayLayers = config.ArrayLayers;
    info.samples = config.Samples;
    info.tiling = config.Tiling;
    info.usage = config.Usage;
    info.sharingMode = config.SharingMode;
    info.initialLayout = config.InitialLayout;
    if (config.SharingMode == VK_SHARING_MODE_CONCURRENT
        && !config.QueueFamilies.empty()) {
        info.queueFamilyIndexCount = static_cast<uint32_t>(config.QueueFamilies.size());
        info.pQueueFamilyIndices = config.QueueFamilies.data();
    }

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = config.MemoryUsage;
    allocInfo.flags = config.AllocationFlags;

    if (vmaCreateImage(device.Allocator, &info, &allocInfo, &out.image,
            &out.allocation_, nullptr)
        != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::ImageCreationFailed,
                "Failed to create image") };
    }

    return { std::move(out), CustomError() };
}

// ============================================================================
// ImageView
// ============================================================================

ImageView::ImageView(ImageView&& other) noexcept
{
    device_ = other.device_;
    view = other.view;
    other.device_ = nullptr;
    other.view = VK_NULL_HANDLE;
}

ImageView& ImageView::operator=(ImageView&& other) noexcept
{
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        view = other.view;
        other.device_ = nullptr;
        other.view = VK_NULL_HANDLE;
    }
    return *this;
}

void ImageView::Destroy()
{
    if (view != VK_NULL_HANDLE && device_) {
        vkDestroyImageView(device_->LogicalDevice, view, nullptr);
        view = VK_NULL_HANDLE;
        device_ = nullptr;
    }
}

std::pair<ImageView, CustomError> CreateImageView(
    Device& device, const Image& image, const ImageViewConfig& config)
{
    ImageView out;
    out.device_ = &device;

    if (image.image == VK_NULL_HANDLE) {
        return { std::move(out),
            CustomError(ErrorCode::ImageViewCreationFailed,
                "Cannot create view of null image") };
    }

    VkImageViewCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image.image;
    info.viewType = config.Type;
    info.format = config.Format == VK_FORMAT_UNDEFINED ? image.format
                                                            : config.Format;
    info.components = config.Components;
    info.subresourceRange.aspectMask = config.AspectMask;
    info.subresourceRange.baseMipLevel = config.BaseMipLevel;
    info.subresourceRange.levelCount = config.LevelCount;
    info.subresourceRange.baseArrayLayer = config.BaseArrayLayer;
    info.subresourceRange.layerCount = config.LayerCount;

    if (vkCreateImageView(device.LogicalDevice, &info, nullptr, &out.view)
        != VK_SUCCESS) {
        return { std::move(out),
            CustomError(ErrorCode::ImageViewCreationFailed,
                "Failed to create image view") };
    }

    return { std::move(out), CustomError() };
}

std::pair<ImageView, CustomError> CreateImageView(
    Device& device, const Image& image,
    VkImageAspectFlags aspectMask, VkFormat format)
{
    auto cfg = ImageViewBuilder(ImageViewPreset::Color2D)
                   .WithAspectMask(aspectMask)
                   .WithFormat(format)
                   .Build();
    return CreateImageView(device, image, cfg);
}

} // namespace yst::core


#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <device/device.hpp>
#include <errors.hpp>
#include <utility>

#include "config.hpp"

namespace yst::core {

/// Owning wrapper around a Vma-allocated VkImage. RAII: destruction
/// automatically frees the underlying VkImage + VmaAllocation.
class Image {
public:
    VkImage image = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent3D extent { 0, 0, 0 };
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;

    Image() = default;
    ~Image() { Destroy(); }

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    /// Free the image. Safe to call multiple times; safe on a moved-from
    /// instance. Uses the device stored at creation time.
    void Destroy();
    bool IsValid() const noexcept { return image != VK_NULL_HANDLE; }

private:
    Device* device_ = nullptr;
    VmaAllocation allocation_ = VK_NULL_HANDLE;

    friend std::pair<Image, CustomError> CreateImage(
        Device& device, const ImageConfig& config);
};

std::pair<Image, CustomError> CreateImage(
    Device& device, const ImageConfig& config);

/// Owning wrapper around a VkImageView. RAII.
class ImageView {
public:
    VkImageView view = VK_NULL_HANDLE;

    ImageView() = default;
    ~ImageView() { Destroy(); }

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;
    ImageView(ImageView&& other) noexcept;
    ImageView& operator=(ImageView&& other) noexcept;

    void Destroy();
    bool IsValid() const noexcept { return view != VK_NULL_HANDLE; }

private:
    Device* device_ = nullptr;

    friend std::pair<ImageView, CustomError> CreateImageView(
        Device& device, const Image& image, const ImageViewConfig& config);
};

std::pair<ImageView, CustomError> CreateImageView(
    Device& device, const Image& image, const ImageViewConfig& config);

/// Convenience overload for 2D color textures.
std::pair<ImageView, CustomError> CreateImageView(
    Device& device, const Image& image,
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    VkFormat format = VK_FORMAT_UNDEFINED);

} // namespace yst::core

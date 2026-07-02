// yst texture module implementation — RAII version (v2)
//
// CreateTexture2D uploads pixel data into a VMA-allocated VkImage, optionally
// generates mipmaps via blits, then creates an ImageView + Sampler around it.
// Returns a Texture2D that owns all three sub-objects and cleans up via RAII.
#include <texture/texture.hpp>

#include <algorithm>
#include <cmath>

#include <buffer/buffer.hpp>
#include <command/command.hpp>
#include <image/image.hpp>
#include <image/sampler.hpp>

namespace yst::core {

Texture2DConfig CreateConfig(Texture2DPreset preset)
{
    Texture2DConfig cfg;
    cfg.Channels = 4;
    cfg.SamplerCfg = CreateConfig(SamplerPreset::LinearRepeat);

    switch (preset) {
    case Texture2DPreset::Rgba8Srgb:
        cfg.Format = VK_FORMAT_R8G8B8A8_SRGB;
        return cfg;

    case Texture2DPreset::MipmappedRgba8Srgb:
        cfg.Format = VK_FORMAT_R8G8B8A8_SRGB;
        cfg.GenerateMipmaps = true;
        return cfg;

    case Texture2DPreset::Rgba8Unorm:
    default:
        cfg.Format = VK_FORMAT_R8G8B8A8_UNORM;
        return cfg;
    }
}

namespace {

    /// Pick a sensible 8-bit unorm VkFormat from a channel count.
    VkFormat PickFormatFromChannels(uint32_t channels)
    {
        switch (channels) {
        case 1:
            return VK_FORMAT_R8_UNORM;
        case 2:
            return VK_FORMAT_R8G8_UNORM;
        case 3:
            return VK_FORMAT_R8G8B8_UNORM;
        case 4:
        default:
            return VK_FORMAT_R8G8B8A8_UNORM;
        }
    }

    /// Compute floor(log2(min(w, h))) + 1 — the number of mip levels that fit
    /// into the given image extent.
    uint32_t ComputeMipLevelCount(uint32_t width, uint32_t height)
    {
        return static_cast<uint32_t>(
                   std::floor(std::log2(std::max<uint32_t>(width, height))))
            + 1;
    }

} // namespace

std::pair<Texture2D, CustomError> CreateTexture2D(
    Device& device, const Texture2DConfig& config)
{
    Texture2D out;

    if (config.Pixels == nullptr || config.Width == 0 || config.Height == 0) {
        return { std::move(out),
            CustomError(ErrorCode::TextureUploadFailed,
                "Texture2DConfig requires non-null Pixels and non-zero extent") };
    }

    const VkFormat format = config.Format == VK_FORMAT_UNDEFINED
        ? PickFormatFromChannels(config.Channels)
        : config.Format;

    const uint32_t mipLevels = config.GenerateMipmaps
        ? ComputeMipLevelCount(config.Width, config.Height)
        : 1;

    // 1. Create the destination image.
    auto imgCfg = ImageBuilder(ImagePreset::Texture2D)
                      .As2DTexture(config.Width, config.Height, format, mipLevels)
                      .Build();
    auto [image, imgErr] = CreateImage(device, imgCfg);
    if (imgErr)
        return { std::move(out), imgErr };
    out.image = std::move(image);

    // 2. Create a staging buffer for the source pixels.
    const VkDeviceSize pixelBytes = static_cast<VkDeviceSize>(config.Width)
        * config.Height * config.Channels;
    auto [staging, stagingErr] = CreateStagingBuffer(device, pixelBytes);
    if (stagingErr) {
        out.image.Destroy();
        return { std::move(out), stagingErr };
    }
    if (auto upErr = staging.UploadData(config.Pixels, pixelBytes)) {
        staging.Destroy();
        out.image.Destroy();
        return { std::move(out), upErr };
    }

    // 3. Submit one-time commands to: transition image to TRANSFER_DST,
    //    copy buffer to image, then either generate mipmaps (which leaves
    //    the image in SHADER_READ_ONLY) or transition to SHADER_READ_ONLY.
    auto submitErr = SubmitOneTimeCommands(device,
        [&](CommandList& cmd) -> CustomError {
            cmd.Begin();

            // Initial transition: UNDEFINED -> TRANSFER_DST_OPTIMAL
            cmd.TransitionImageLayout(out.image.image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

            // Copy the staging buffer into mip level 0.
            cmd.CopyBufferToImage(staging.buffer, out.image.image,
                config.Width, config.Height);

            if (config.GenerateMipmaps && mipLevels > 1) {
                // Generate remaining mip levels via blits from level 0.
                //
                // Loop invariant: at the start of iteration i, mip level i-1
                // is in TRANSFER_DST_OPTIMAL and all lower-numbered mips are
                // in TRANSFER_SRC_OPTIMAL. We bump level i-1 to SRC, blit it
                // into level i (which leaves i in DST), then on the next
                // iteration we'll bump i to SRC, etc.
                uint32_t w = config.Width;
                uint32_t h = config.Height;
                for (uint32_t i = 1; i < mipLevels; ++i) {
                    cmd.PipelineBarrierImage(out.image.image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT,
                        /*baseMipLevel=*/i - 1,
                        /*levelCount=*/1,
                        /*baseArrayLayer=*/0,
                        /*layerCount=*/1);

                    const uint32_t mipW = std::max(1u, w / 2);
                    const uint32_t mipH = std::max(1u, h / 2);

                    cmd.BlitImageMip(out.image.image, out.image.image,
                        w, h, /*srcMip=*/i - 1,
                        mipW, mipH, /*dstMip=*/i,
                        VK_FILTER_LINEAR);

                    w = mipW;
                    h = mipH;
                }

                // After the loop:
                //   - mip levels 0 .. mipLevels-2 are in TRANSFER_SRC_OPTIMAL
                //   - mip level mipLevels-1 is in TRANSFER_DST_OPTIMAL
                // We need to bring all of them to SHADER_READ_ONLY_OPTIMAL.
                for (uint32_t i = 0; i < mipLevels; ++i) {
                    const VkImageLayout oldLayout = (i == mipLevels - 1)
                        ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                        : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    const VkAccessFlags srcAccess = (i == mipLevels - 1)
                        ? VK_ACCESS_TRANSFER_WRITE_BIT
                        : VK_ACCESS_TRANSFER_READ_BIT;
                    cmd.PipelineBarrierImage(out.image.image,
                        oldLayout,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        srcAccess,
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT,
                        /*baseMipLevel=*/i,
                        /*levelCount=*/1,
                        /*baseArrayLayer=*/0,
                        /*layerCount=*/1);
                }
            } else {
                // No mipmap generation — transition all levels directly
                // from TRANSFER_DST_OPTIMAL to SHADER_READ_ONLY_OPTIMAL.
                cmd.TransitionImageLayout(out.image.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
            }

            cmd.End();
            return CustomError();
        });

    staging.Destroy();
    if (submitErr) {
        out.image.Destroy();
        return { std::move(out), submitErr };
    }

    // 4. Create the image view.
    auto viewCfg = ImageViewBuilder(ImageViewPreset::Color2D)
                       .WithAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
                       .WithType(VK_IMAGE_VIEW_TYPE_2D)
                       .Build();
    auto [view, viewErr] = CreateImageView(device, out.image, viewCfg);
    if (viewErr) {
        out.image.Destroy();
        return { std::move(out), viewErr };
    }
    out.view = std::move(view);

    // 5. Create the sampler.
    auto [sampler, samplerErr] = CreateSampler(device, config.SamplerCfg);
    if (samplerErr) {
        out.view.Destroy();
        out.image.Destroy();
        return { std::move(out), samplerErr };
    }
    out.sampler = std::move(sampler);

    return { std::move(out), CustomError() };
}

} // namespace yst::core


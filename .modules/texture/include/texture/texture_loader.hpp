#pragma once
#include <device/device.hpp>
#include <errors.hpp>
#include <string>
#include <utility>
#include <vector>

namespace yst::core {

/// Decoded pixel data returned by LoadStbImage. Stores width/height/channels
/// and a heap-allocated pixel buffer (8 bits per channel, RGBA forced to 4
/// channels for simplicity — the underlying stb_image always decodes to 4).
struct LoadedPixels {
    std::vector<uint8_t> bytes;
    int Width = 0;
    int Height = 0;
    int Channels = 4; // stb_image is forced to RGBA
};

/// Load an image file (PNG / JPEG / BMP / PSD / TGA / etc. — anything
/// stb_image supports) into RGBA8 CPU memory. Returns
/// TextureFileOpenFailed / TextureFileDecodeFailed on error.
std::pair<LoadedPixels, CustomError> LoadStbImage(const std::string& path);

} // namespace yst::core

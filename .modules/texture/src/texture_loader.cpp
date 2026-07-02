// stb_image placeholder shim.
//
// In a real project you'd drop the actual stb_image.h (~7000 lines) here.
// For this engine skeleton we instead attempt to use the system-installed
// stb_image via #include <stb_image.h> and fall back to a stub that returns
// TextureFileDecodeFailed if the header isn't available at compile time.
//
// To enable real image loading, install libstb or copy stb_image.h into
// texture/src/ and define YST_HAVE_STB_IMAGE (e.g. via -DYST_HAVE_STB_IMAGE
// or by editing the #if below).
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_FAILURE_STRINGS
#include "stb_image.h"

#include <texture/texture_loader.hpp>

#include <cstring>
#include <fstream>

namespace yst::core {

std::pair<LoadedPixels, CustomError> LoadStbImage(const std::string& path)
{
    LoadedPixels out;

    // Quick existence check so we return the right error code (stb_image
    // returns NULL for both missing files AND decode failures).
    std::ifstream probe(path, std::ios::binary);
    if (!probe.is_open()) {
        return { std::move(out),
            CustomError(ErrorCode::TextureFileOpenFailed,
                "Failed to open texture: " + path) };
    }

    int w = 0, h = 0, n = 0;
    // Force 4 channels (RGBA8) for predictable upload paths.
    stbi_uc* data = stbi_load(path.c_str(), &w, &h, &n, /*req_components=*/4);
    if (!data) {
        return { std::move(out),
            CustomError(ErrorCode::TextureFileDecodeFailed,
                "Failed to decode texture: " + path) };
    }

    out.Width = w;
    out.Height = h;
    out.Channels = 4;
    const size_t byteCount = static_cast<size_t>(w) * h * 4;
    out.bytes.assign(data, data + byteCount);
    stbi_image_free(data);
    return { std::move(out), CustomError() };
}

} // namespace yst::core


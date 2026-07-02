#pragma once
#include <enums.hpp>
#include <string>

namespace yst {

/// Lightweight, copyable error type used across the engine.
///
/// `code == 0` means success. Any non-zero value indicates failure.
/// `code` may carry either an `ErrorCode` (cast to int) or, in the rare case
/// where we forward a Vulkan result, the raw negative `VkResult` value
/// (positive VkResult values like VK_SUBOPTIMAL_KHR are also forwarded).
class CustomError {
    std::string message;

public:
    int code;

    [[nodiscard]] std::string str() const;

    explicit operator bool() const { return code != 0; }

    /// Construct from a raw int code. Preserved for backward compatibility
    /// and for forwarding arbitrary VkResult values.
    CustomError(int code, const std::string& message)
        : code(code)
        , message(message)
    {
    }

    /// Construct from a typed ErrorCode.
    CustomError(ErrorCode ec, const std::string& message)
        : code(static_cast<int>(ec))
        , message(message)
    {
    }

    CustomError()
        : code(0)
        , message("")
    {
    }

    /// Compare against typed ErrorCode values.
    bool Is(ErrorCode ec) const noexcept { return code == static_cast<int>(ec); }

    static CustomError Unknown()
    {
        return CustomError(ErrorCode::Unknown, "Something went wrong");
    }

    const char* name() const;

    ~CustomError() = default;
};

} // namespace yst

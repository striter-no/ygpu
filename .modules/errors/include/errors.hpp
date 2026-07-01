#pragma once
#include <string>

namespace yst {
class CustomError {
    std::string message;

public:
    int code;

    [[nodiscard]] std::string str() const;

    explicit operator bool() const { return code != 0; }

    CustomError(int code, const std::string& message)
        : code(code)
        , message(message)
    {
    }

    CustomError()
        : code(0)
        , message("")
    {
    }

    static CustomError Unknown()
    {
        return CustomError(-1, "Something went wrong");
    }

    ~CustomError() = default;
};

} // namespace yst

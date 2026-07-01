#include <errors.hpp>

namespace yst {
std::string CustomError::str() const
{
    if (code == 0)
        return "[no errors]";

    return "[" + std::to_string(code) + "] Error: " + message;
}
} // namespace yst

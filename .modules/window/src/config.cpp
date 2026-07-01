#include "window/config.hpp"

namespace yst::ywin {
WindowConfig CreateConfig(ywinc::Preset preset)
{
    switch (preset) {
    case yst::ywinc::DEFAULT_WINDOW:
        return ywin::WindowConfig {};
    default:
        return ywin::WindowConfig {};
    }
}
} // namespace yst::ywin

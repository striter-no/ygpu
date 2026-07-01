#include "window/config.hpp"

namespace yst::ywin {

WindowConfig CreateConfig(ywinc::Preset preset)
{
    WindowConfig cfg;
    switch (preset) {
    case ywinc::DEFAULT_WINDOW:
        // Already matches WindowConfig{} defaults (1280x720 resizable decorated).
        return cfg;

    case ywinc::FULLSCREEN_BORDERLESS:
        cfg.Maximized = true;
        cfg.Decorated = false;
        cfg.Resizable = false;
        cfg.Focused = true;
        // Note: actual borderless-fullscreen on a specific monitor is set up
        // by the caller via glfwSetWindowMonitor; the config here just makes
        // the resulting window maximised and undecorated, which is the
        // conventional "borderless windowed" look.
        return cfg;

    case ywinc::HEADLESS_RENDER:
        cfg.Visible = false;
        cfg.Decorated = false;
        cfg.Resizable = false;
        cfg.Width = 1;
        cfg.Height = 1;
        return cfg;

    case ywinc::FIXED_DEBUG:
        cfg.Width = 800;
        cfg.Height = 600;
        cfg.Resizable = false;
        cfg.Title = "YST Debug";
        return cfg;
    }
    return cfg;
}

} // namespace yst::ywin

#pragma once
#include <optional>
#include <string>
#include <vector>

namespace yst::ywinc {

/// Named presets for typical window configurations.
/// Used as the "Layer 1" entry point — callers that don't want to fill in
/// every field of WindowConfig can ask for a preset via CreateConfig(preset).
enum Preset {
    DEFAULT_WINDOW = 0, ///< 1280x720, decorated, resizable, visible.
    FULLSCREEN_BORDERLESS, ///< Fullscreen at the primary monitor's desktop res.
    HEADLESS_RENDER, ///< Hidden window (for offscreen compute/capture).
    FIXED_DEBUG, ///< 800x600, non-resizable, for reproducible test runs.
};

/// GLFW window hint identifiers. We mirror the GLFW_* integer constants so
/// callers don't have to pull <GLFW/glfw3.h> into headers just to set a hint.
/// Values intentionally match GLFW's enum to keep the lookup trivial.
enum WindowHint : int {
    HintClientApi = 0x00022001, // GLFW_CLIENT_API
    HintResizable = 0x00020003, // GLFW_RESIZABLE
    HintVisible = 0x00020004, // GLFW_VISIBLE
    HintDecorated = 0x00020005, // GLFW_DECORATED
    HintFocused = 0x00020001, // GLFW_FOCUSED
    HintAutoIconify = 0x00020006, // GLFW_AUTO_ICONIFY
    HintFloating = 0x00020007, // GLFW_FLOATING
    HintMaximized = 0x00020008, // GLFW_MAXIMIZED
    HintCenterCursor = 0x00020009, // GLFW_CENTER_CURSOR
    HintTransparentFramebuffer = 0x0002000A, // GLFW_TRANSPARENT_FRAMEBUFFER
    HintFocusOnShow = 0x0002000C, // GLFW_FOCUS_ON_SHOW
    HintScaleToMonitor = 0x0002200C, // GLFW_SCALE_TO_MONITOR
    HintRefreshRate = 0x0002100F, // GLFW_REFRESH_RATE
};

} // namespace yst::ywinc

namespace yst::ywin {

/// Layer 2 configuration for a Window.
///
/// Every field has a default that matches the historical hardcoded behaviour,
/// so `WindowConfig{}` produces the same window the engine used to create
/// before this struct existed. Callers can override any subset of fields,
/// and may append raw GLFW hints (key/value pairs from ywinc::WindowHint)
/// for advanced cases not covered by the named fields.
struct WindowConfig {
    int Width = 1280;
    int Height = 720;
    std::string Title = "YST Engine";
    bool Resizable = true;
    bool Visible = true;
    bool Decorated = true;
    bool Focused = true;
    bool Maximized = false;
    bool TransparentFramebuffer = false;
    bool Floating = false; ///< Always-on-top.
    bool ScaleToMonitor = false;

    /// Refresh rate hint, in Hz. nullopt = use the monitor's desktop mode.
    std::optional<int> RefreshRate;

    /// For monitor-fullscreen mode: nullptr = windowed. Pass a non-null
    /// monitor name to enable fullscreen on that monitor.
    /// (Currently informational; the public CreateWindow API in this module
    /// is windowed-only, but the field is preserved so config files can
    /// describe fullscreen targets without a struct redesign.)
    std::string FullscreenMonitorName;

    /// Escape hatch: arbitrary GLFW hint overrides applied after the named
    /// fields above. Each entry is {hint, value}. Use values from
    /// ywinc::WindowHint for the first element.
    std::vector<std::pair<int, int>> ExtraHints;
};

/// Resolve a named preset into a fully-populated WindowConfig.
/// Returns a value, not a reference, so callers can tweak individual fields
/// afterwards: `auto cfg = CreateConfig(FIXED_DEBUG); cfg.Title = "Tests";`
WindowConfig CreateConfig(ywinc::Preset preset);

} // namespace yst::ywin


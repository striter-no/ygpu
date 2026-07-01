#pragma once
#include <vulkan/vulkan_core.h>

#include <errors.hpp>

#include "config.hpp"

struct GLFWwindow;

namespace yst::ywin {

class Window {
private:
    bool isResized = false;

    GLFWwindow* handle = nullptr;
    mutable VkSurfaceKHR cachedSurface = VK_NULL_HANDLE;

    explicit Window(GLFWwindow* ptr)
        : handle(ptr)
    {
    }

    /// Re-bind the GLFW window user pointer to `this`. Called by the move
    /// special members so the resize callback never sees a stale address.
    void RebindUserPointer() noexcept;

    friend std::pair<Window, CustomError> CreateWindow(
        const WindowConfig& config);

public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    void Destroy();

    bool IsOpen() const;
    void PollEvents() const;
    void WaitEvents() const;
    bool IsMinimized() const;
    bool IsResized() const;

    void MarkResized();
    void ClearResizeFlag();

    /// Get the current framebuffer size in pixels.
    /// Returns {0,0} if the window has no handle or is minimised.
    struct Extent2D {
        int Width;
        int Height;
    };
    Extent2D GetFramebufferSize() const;

    GLFWwindow* GetHandle() const;
    VkSurfaceKHR GetSurface(VkInstance instance) const;
};

std::pair<Window, CustomError> CreateWindow(const WindowConfig& config);

/// Convenience overload (Layer 0 of progressive disclosure): construct a
/// window with the DEFAULT_WINDOW preset. Equivalent to
/// `CreateWindow(CreateConfig(ywinc::DEFAULT_WINDOW))`.
inline std::pair<Window, CustomError> CreateWindow()
{
    return CreateWindow(CreateConfig(ywinc::DEFAULT_WINDOW));
}

} // namespace yst::ywin

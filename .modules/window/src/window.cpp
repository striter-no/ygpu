#include <volk.h>

#include <GLFW/glfw3.h>
#include <iostream>
#include <window/window.hpp>

namespace yst::ywin {

namespace {

    /// Apply the named config fields + ExtraHints to the current GLFW context.
    void ApplyHints(const WindowConfig& config)
    {
        // The historical hardcoded hint: no OpenGL / GL context, just Vulkan.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, config.Resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, config.Visible ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, config.Decorated ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUSED, config.Focused ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_MAXIMIZED, config.Maximized ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, config.Floating ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER,
            config.TransparentFramebuffer ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_SCALE_TO_MONITOR,
            config.ScaleToMonitor ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_REFRESH_RATE,
            config.RefreshRate.value_or(GLFW_DONT_CARE));

        // Caller-supplied escape hatch. Applied last so they override the named
        // fields if the caller deliberately duplicates a hint.
        for (const auto& [hint, value] : config.ExtraHints) {
            glfwWindowHint(hint, value);
        }
    }

} // namespace

std::pair<Window, CustomError> CreateWindow(const WindowConfig& config, const yst::core::Device& device)
{
#ifdef __APPLE__
    glfwInitVulkanLoader(device.vkGetInstanceProcAddr);
    std::cerr << "[debug] called glfwInitVulkanLoader with custom vkGetInstanceProcAddr\n";
#endif

    if (!glfwInit())
        return { Window {},
            CustomError(ErrorCode::WindowGLFWInitFailed,
                "Failed to initialize GLFW") };

    ApplyHints(config);

    GLFWwindow* glfwWindow = glfwCreateWindow(
        config.Width, config.Height, config.Title.c_str(), nullptr, nullptr);

    if (!glfwWindow) {
        glfwTerminate();
        return { Window {},
            CustomError(ErrorCode::WindowCreationFailed,
                "Failed to create GLFW Window") };
    }

    Window win { glfwWindow };

    // IMPORTANT: store the address of the heap/stable Window object that the
    // caller will own, NOT the address of `win` (a local that will be moved
    // from on return). The move constructor/assignment update this pointer
    // every time the Window is relocated, so the callback never sees a
    // dangling pointer.
    glfwSetWindowUserPointer(glfwWindow, &win);
    glfwSetFramebufferSizeCallback(glfwWindow,
        [](GLFWwindow* w, int, int) {
            auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
            if (self)
                self->MarkResized();
        });

    return { std::move(win), CustomError {} };
}

VkSurfaceKHR Window::GetSurface(VkInstance instance) const
{
    if (cachedSurface != VK_NULL_HANDLE) {
        return cachedSurface;
    }
    if (!handle || instance == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }
    if (glfwCreateWindowSurface(instance, handle, nullptr, &cachedSurface)
        != VK_SUCCESS) {
        cachedSurface = VK_NULL_HANDLE;
    }
    return cachedSurface;
}

GLFWwindow* Window::GetHandle() const { return handle; }

void Window::Destroy()
{
    if (handle) {
        glfwDestroyWindow(handle);
        handle = nullptr;
    }
}

Window::~Window() { Destroy(); }

void Window::RebindUserPointer() noexcept
{
    if (handle) {
        glfwSetWindowUserPointer(handle, this);
    }
}

Window::Window(Window&& other) noexcept
{
    handle = other.handle;
    isResized = other.isResized;
    cachedSurface = other.cachedSurface;

    other.handle = nullptr;
    other.cachedSurface = VK_NULL_HANDLE;
    other.isResized = false;

    RebindUserPointer();
}

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other) {
        Destroy();
        handle = other.handle;
        isResized = other.isResized;
        cachedSurface = other.cachedSurface;

        other.handle = nullptr;
        other.cachedSurface = VK_NULL_HANDLE;
        other.isResized = false;

        RebindUserPointer();
    }
    return *this;
}

bool Window::IsOpen() const
{
    if (!handle)
        return false;
    return !glfwWindowShouldClose(handle);
}

void Window::PollEvents() const { glfwPollEvents(); }
void Window::WaitEvents() const { glfwWaitEvents(); }

bool Window::IsMinimized() const
{
    auto [w, h] = GetFramebufferSize();
    return w == 0 || h == 0;
}

bool Window::IsResized() const { return isResized; }

void Window::MarkResized() { isResized = true; }

void Window::ClearResizeFlag() { isResized = false; }

Window::Extent2D Window::GetFramebufferSize() const
{
    if (!handle)
        return { 0, 0 };
    int w = 0, h = 0;
    glfwGetFramebufferSize(handle, &w, &h);
    return { w, h };
}

} // namespace yst::ywin

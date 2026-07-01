#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <window.hpp>

namespace yst::ywin {

std::pair<Window, CustomError> CreateWindow(const WindowConfig &config) {
  if (!glfwInit()) {
    return {Window{}, CustomError(1, "Failed to initialize GLFW")};
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, config.Resizable ? GLFW_TRUE : GLFW_FALSE);

  GLFWwindow *glfwWindow = glfwCreateWindow(
      config.Width, config.Height, config.Title.c_str(), nullptr, nullptr);

  if (!glfwWindow) {
    glfwTerminate();
    return {Window{}, CustomError(2, "Failed to create GLFW Window")};
  }

  return {Window{glfwWindow}, CustomError{}};
}

VkSurfaceKHR Window::CreateVulkanSurface(VkInstance instance) const {
  VkSurfaceKHR surface = VK_NULL_HANDLE;

  if (glfwCreateWindowSurface(instance, handle, nullptr, &surface) !=
      VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  return surface;
}

void Window::Destroy() {
  if (handle) {
    glfwDestroyWindow(handle);
    handle = nullptr;
  }
}

Window::~Window() { Destroy(); }

Window::Window(Window &&other) noexcept {
  handle = other.handle;
  other.handle = nullptr;
}

Window &Window::operator=(Window &&other) noexcept {
  if (this != &other) {
    Destroy();
    handle = other.handle;
    other.handle = nullptr;
  }
  return *this;
}

bool Window::IsOpen() const {
  if (!handle)
    return false;
  return !glfwWindowShouldClose(handle);
}

void Window::PollEvents() const { glfwPollEvents(); }

bool Window::IsMinimized() const {
  if (!handle)
    return false;
  int w = 0, h = 0;
  glfwGetFramebufferSize(handle, &w, &h);
  return w == 0 || h == 0;
}
} // namespace yst::ywin

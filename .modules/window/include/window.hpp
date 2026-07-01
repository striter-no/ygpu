#pragma once
#include <errors.hpp>
#include <string>
#include <vulkan/vulkan_core.h>

struct GLFWwindow;

namespace yst::ywin {

struct WindowConfig {
  int Width = 800;
  int Height = 600;
  std::string Title = "YST Engine";
  bool Resizable = true;
};

class Window {
private:
  GLFWwindow *handle = nullptr;

  explicit Window(GLFWwindow *ptr) : handle(ptr) {}
  friend std::pair<Window, CustomError>
  CreateWindow(const WindowConfig &config);

public:
  Window() = default;
  ~Window();

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  Window(Window &&other) noexcept;
  Window &operator=(Window &&other) noexcept;

  void Destroy();

  bool IsOpen() const;
  void PollEvents() const;
  bool IsMinimized() const;

  GLFWwindow *GetHandle() const { return handle; }
  VkSurfaceKHR CreateVulkanSurface(VkInstance instance) const;
};

std::pair<Window, CustomError> CreateWindow(const WindowConfig &config);
} // namespace yst::ywin

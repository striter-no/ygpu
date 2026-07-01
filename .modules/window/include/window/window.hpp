#pragma once
#include "config.hpp"
#include <errors.hpp>
#include <vulkan/vulkan_core.h>

struct GLFWwindow;

namespace yst::ywin {
	class Window {
	private:
		bool isResized;

		GLFWwindow *handle = nullptr;
		mutable VkSurfaceKHR cachedSurface = VK_NULL_HANDLE;

		explicit Window(GLFWwindow *ptr) : handle(ptr) {}
		friend std::pair<Window, CustomError> CreateWindow(const WindowConfig &config);

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
		bool IsResized() const;

		void MarkResized();
		void ClearResizeFlag();

		GLFWwindow *GetHandle() const;
		VkSurfaceKHR GetSurface(VkInstance instance) const;
	};

	std::pair<Window, CustomError> CreateWindow(const WindowConfig &config);
} // namespace yst::ywin

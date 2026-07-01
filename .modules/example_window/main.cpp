#include <iostream>
#include <window.hpp>

int main() {
  yst::ywin::WindowConfig config;
  config.Width = 1024;
  config.Height = 768;
  config.Title = "YST Window Test";

  auto [window, error] = yst::ywin::CreateWindow(config);

  if (error) {
    std::cerr << "Window Error: " << error.str() << std::endl;
    return -1;
  }

  std::cout << "Window created successfully! Close it to exit." << std::endl;

  while (window.IsOpen()) {
    window.PollEvents();
  }

  std::cout << "Window closed naturally." << std::endl;
  return 0;
}

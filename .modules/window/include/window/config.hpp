#pragma once
#include <string>

namespace yst::ywinc {
    enum Preset { DEFAULT_WINDOW };
}

namespace yst::ywin {

	struct WindowConfig {
		int Width = 800;
		int Height = 600;
		std::string Title = "YST Engine";
		bool Resizable = true;
	};

	WindowConfig CreateConfig(ywinc::Preset preset);
}

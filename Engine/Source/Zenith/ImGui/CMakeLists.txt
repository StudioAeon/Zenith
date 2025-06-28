set(IMGUI_SOURCES
		ImGuiCore.cpp
		ImGuiFonts.cpp
		ImGuiLayer.cpp
)

set(IMGUI_HEADERS
		Colors.hpp
		ImGuiCore.hpp
		ImGuiFonts.hpp
		ImGuiLayer.hpp
)

target_sources(Zenith
		PRIVATE
		${IMGUI_SOURCES}
		${IMGUI_HEADERS}
)
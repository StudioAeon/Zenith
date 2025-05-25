#pragma once

#include <string>
#include <SDL3/SDL.h>

namespace Zenith {

	class Window {
	public:
		Window(const std::string& title, int width, int height);
		~Window();

		void PollEvents() const;
		bool ShouldClose() const;

		SDL_Window* GetNativeWindow() const { return m_Window; }

	private:
		SDL_Window* m_Window = nullptr;
		SDL_Event m_Event{};
		bool m_ShouldClose = false;
	};

}
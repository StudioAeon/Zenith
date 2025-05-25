#include "znpch.hpp"
#include "Window.hpp"

#include <iostream>

namespace Zenith {

	Window::Window(const std::string& title, int width, int height) {

		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
				ZN_ASSERT("SDL_Init failed: {}", SDL_GetError());
		}

		m_Window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
		if (!m_Window) {
				ZN_ASSERT("SDL_CreateWindow failed: {}", SDL_GetError());
		}

		ZN_CORE_INFO("Window created: {} ({}x{})", title, width, height);
	}

	Window::~Window() {
		if (m_Window) {
			SDL_DestroyWindow(m_Window);
		}
		SDL_Quit();
	}

	void Window::PollEvents() const {
		SDL_PumpEvents();
		SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_EVENT_QUIT) {
					const_cast<Window*>(this)->m_ShouldClose = true;
				}
		}
	}

	bool Window::ShouldClose() const {
		return m_ShouldClose;
	}

}
#include "znpch.hpp"
#include "Window.hpp"

#include "Application.hpp"

#include "Zenith/Events/ApplicationEvent.hpp"
#include "Zenith/Events/KeyEvent.hpp"
#include "Zenith/Events/MouseEvent.hpp"
#include "Zenith/Core/Input.hpp"

namespace Zenith {

	static bool s_SDLInitialized = false;

	Window* Window::Create(const WindowSpecification& specification)
	{
		return new Window(specification);
	}
	
	Window::Window(const WindowSpecification& specification)
		: m_Specification(specification)
	{}

	Window::~Window()
	{
		Shutdown();
	}

	void Window::Init()
	{
		m_Data.Title = m_Specification.Title;
		m_Data.Width = m_Specification.Width;
		m_Data.Height = m_Specification.Height;

		ZN_CORE_INFO_TAG("SDL", "Creating window {} ({}x{})", m_Data.Title, m_Data.Width, m_Data.Height);

		if (!s_SDLInitialized)
		{
			if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
			{
				ZN_CORE_ASSERT(SDL_Init(SDL_INIT_VIDEO), "Could not initialize SDL: {}", SDL_GetError());
			}
			s_SDLInitialized = true;
		}

		Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

		if (m_Specification.Fullscreen)
			windowFlags |= SDL_WINDOW_FULLSCREEN;

		switch (RendererAPI::Current())
		{
			case RendererAPIType::OpenGL:
				windowFlags |= SDL_WINDOW_OPENGL;
				break;
			case RendererAPIType::Vulkan:
				windowFlags |= SDL_WINDOW_VULKAN;
				break;
			case RendererAPIType::None:
				ZN_CORE_ASSERT(false, "RendererAPI::None is not supported");
				break;
		}

		m_Window = SDL_CreateWindow(m_Data.Title.c_str(), m_Data.Width, m_Data.Height, windowFlags);

		if (!m_Window) {
			ZN_CORE_ASSERT(m_Window != nullptr, "Failed to create SDL Window: {}", SDL_GetError());
		}

		// Create Renderer Context
		m_RendererContext = RendererContext::Create(m_Window);
		m_RendererContext->Create();

		SetVSync(m_Specification.VSync);

		// Update window size to actual value (just in case)
		int w, h;
		SDL_GetWindowSize(m_Window, &w, &h);
		m_Data.Width = w;
		m_Data.Height = h;
	}

	void Window::Shutdown()
	{
		if (m_Window) {
			SDL_DestroyWindow(m_Window);
			m_Window = nullptr;
		}
		SDL_Quit();
		s_SDLInitialized = false;
	}

	std::pair<float, float> Window::GetWindowPos() const
	{
		int x, y;
		SDL_GetWindowPosition(m_Window, &x, &y);
		return { static_cast<float>(x), static_cast<float>(y) };
	}

	void Window::ProcessEvents()
	{
		PollEvents();
		Input::Update();
	}

	void Window::PollEvents()
	{
		while (SDL_PollEvent(&m_Event))
		{
			// Let the Input system process input-related events first
			Input::ProcessEvent(m_Event);

			switch (m_Event.type) {
					case SDL_EVENT_QUIT: {
					auto e = std::make_unique<WindowCloseEvent>();
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				case SDL_EVENT_WINDOW_RESIZED: {
					auto e = std::make_unique<WindowResizeEvent>(m_Event.window.data1, m_Event.window.data2);
					m_Data.Width = m_Event.window.data1;
					m_Data.Height = m_Event.window.data2;
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				case SDL_EVENT_WINDOW_MINIMIZED: {
					auto e = std::make_unique<WindowMinimizeEvent>(true);
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				case SDL_EVENT_KEY_DOWN: {
					int repeatCount = m_Event.key.repeat ? 1 : 0;
					auto e = std::make_unique<KeyPressedEvent>(m_Event.key.scancode, repeatCount);
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				case SDL_EVENT_KEY_UP: {
					auto e = std::make_unique<KeyReleasedEvent>(m_Event.key.scancode);
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				case SDL_EVENT_TEXT_INPUT: {
					auto e = std::make_unique<KeyTypedEvent>(m_Event.text.text[0]);
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_DOWN: {
					auto e = std::make_unique<MouseButtonPressedEvent>(m_Event.button.button);
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_UP: {
					auto e = std::make_unique<MouseButtonReleasedEvent>(m_Event.button.button);
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				case SDL_EVENT_MOUSE_MOTION: {
					auto e = std::make_unique<MouseMovedEvent>(
						static_cast<float>(m_Event.motion.x),
						static_cast<float>(m_Event.motion.y));
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				case SDL_EVENT_MOUSE_WHEEL: {
					auto e = std::make_unique<MouseScrolledEvent>(
						static_cast<float>(m_Event.wheel.x),
						static_cast<float>(m_Event.wheel.y));
					Application::Get().GetEventBus().QueueEvent(std::move(e));
					break;
				}
				// Let Input system fully handle controller events
			}
		}
	}

	void Window::SwapBuffers()
	{
		m_RendererContext->SwapBuffers();
	}

	void Window::SetVSync(bool enabled)
	{
		m_Specification.VSync = enabled;
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			SDL_GL_SetSwapInterval(enabled ? 1 : 0);
		}
	}

	bool Window::IsVSync() const
	{
		return m_Specification.VSync;
	}

	void Window::SetResizable(bool resizable) const
	{
		SDL_SetWindowResizable(m_Window, resizable ? true : false);
	}

	void Window::Maximize()
	{
		SDL_MaximizeWindow(m_Window);
	}

	void Window::CenterWindow()
	{
		SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);

		if (!mode)
		{
			ZN_CORE_ERROR_TAG("SDL","Failed to get current display mode: {}", SDL_GetError());
			return;
		}

		int x = (mode->w / 2) - (m_Data.Width / 2);
		int y = (mode->h / 2) - (m_Data.Height / 2);
		SDL_SetWindowPosition(m_Window, x, y);
	}

	void Window::SetTitle(const std::string& title)
	{
		m_Data.Title = title;
		SDL_SetWindowTitle(m_Window, title.c_str());
	}

}
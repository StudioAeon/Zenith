#include "znpch.hpp"
#include "Window.hpp"

#include "Application.hpp"

#include "Zenith/Events/ApplicationEvent.hpp"
#include "Zenith/Events/KeyEvent.hpp"
#include "Zenith/Events/MouseEvent.hpp"
#include "Zenith/Core/Input.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanSwapChain.hpp"

#include <stb/stb_image.h>
#include <backends/imgui_impl_sdl3.h>

namespace Zenith {

#include "Zenith/Embed/ZenithIcon.embed"

	static bool s_SDLInitialized = false;

	std::unique_ptr<Window> Window::Create(const WindowSpecification& specification)
	{
		auto window = std::make_unique<Window>(specification);
		window->Init();
		return window;
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
			if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK) != 0)
			{
				ZN_CORE_ASSERT(SDL_Init(SDL_INIT_VIDEO), "Could not initialize SDL: {}", SDL_GetError());
			}
			s_SDLInitialized = true;
		}

		Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

		if (m_Specification.Resizable)
			windowFlags |= SDL_WINDOW_RESIZABLE;

		if (m_Specification.Fullscreen)
			windowFlags |= SDL_WINDOW_FULLSCREEN;

		if (m_Specification.Maximized)
			windowFlags |= SDL_WINDOW_MAXIMIZED;

		switch (RendererAPI::Current())
		{
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

		// Set icon
		{
			int width, height, channels;
			unsigned char* pixels = nullptr;

			bool useEmbedded = m_Specification.IconPath.empty();

			if (!useEmbedded)
			{
				std::string iconPathStr = m_Specification.IconPath.string();
				pixels = stbi_load(iconPathStr.c_str(), &width, &height, &channels, 4);
				if (!pixels)
				{
					ZN_CORE_WARN_TAG("SDL", "Failed to load custom icon from: {}", iconPathStr);
					useEmbedded = true;
				}
			}

			if (useEmbedded)
			{
				pixels = stbi_load_from_memory(g_ZenithIconPNG, sizeof(g_ZenithIconPNG), &width, &height, &channels, 4);
			}

			if (pixels)
			{
				SDL_Surface* iconSurface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, (void*)pixels, width * 4);

				if (iconSurface)
				{
					SDL_SetWindowIcon(m_Window, iconSurface);
					SDL_DestroySurface(iconSurface);
				}
				else
				{
					ZN_CORE_ERROR_TAG("SDL", "Failed to create icon surface: {}", SDL_GetError());
				}

				stbi_image_free(pixels);
			}
			else
			{
				ZN_CORE_WARN_TAG("SDL", "No icon could be loaded");
			}
		}

		// Create Renderer Context
		m_RendererContext = RendererContext::Create();
		m_RendererContext->Init();

		Ref<VulkanContext> context = m_RendererContext.As<VulkanContext>();

		m_SwapChain = znew VulkanSwapChain();
		m_SwapChain->Init(VulkanContext::GetInstance(), context->GetDevice());
		m_SwapChain->InitSurface(m_Window);

		m_SwapChain->Create(&m_Data.Width, &m_Data.Height, m_Specification.VSync);

		// Update window size to actual value (just in case)
		int w, h;
		SDL_GetWindowSize(m_Window, &w, &h);
		m_Data.Width = w;
		m_Data.Height = h;
	}

	void Window::Shutdown()
	{
		Input::Shutdown();

		m_SwapChain->Destroy();
		zdelete m_SwapChain;
		m_RendererContext.As<VulkanContext>()->GetDevice()->Destroy(); // need to destroy the device _before_ windows window destructor destroys the renderer context (because device Destroy() asks for renderer context...)
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
			if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendPlatformUserData != nullptr)
			{
				ImGui_ImplSDL3_ProcessEvent(&m_Event);
			}

			Input::ProcessEvent(m_Event);

			switch (m_Event.type) {
				case SDL_EVENT_QUIT: {
					auto e = std::make_unique<WindowCloseEvent>();
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				case SDL_EVENT_WINDOW_RESIZED: {
					auto e = std::make_unique<WindowResizeEvent>(m_Event.window.data1, m_Event.window.data2);
					m_Data.Width = m_Event.window.data1;
					m_Data.Height = m_Event.window.data2;
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				case SDL_EVENT_WINDOW_MINIMIZED: {
					auto e = std::make_unique<WindowMinimizeEvent>(true);
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				case SDL_EVENT_KEY_DOWN: {
					int repeatCount = m_Event.key.repeat ? 1 : 0;
					auto e = std::make_unique<KeyPressedEvent>(m_Event.key.scancode, repeatCount);
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				case SDL_EVENT_KEY_UP: {
					auto e = std::make_unique<KeyReleasedEvent>(m_Event.key.scancode);
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				case SDL_EVENT_TEXT_INPUT: {
					auto e = std::make_unique<KeyTypedEvent>(m_Event.text.text[0]);
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_DOWN: {
					auto e = std::make_unique<MouseButtonPressedEvent>(m_Event.button.button);
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_UP: {
					auto e = std::make_unique<MouseButtonReleasedEvent>(m_Event.button.button);
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				case SDL_EVENT_MOUSE_MOTION: {
					auto e = std::make_unique<MouseMovedEvent>(
						static_cast<float>(m_Event.motion.x),
						static_cast<float>(m_Event.motion.y));
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				case SDL_EVENT_MOUSE_WHEEL: {
					auto e = std::make_unique<MouseScrolledEvent>(
						static_cast<float>(m_Event.wheel.x),
						static_cast<float>(m_Event.wheel.y));
					if (m_Data.EventCallback) {
						m_Data.EventCallback(*e);
					}
					break;
				}
				// Let Input system fully handle controller events
			}
		}
	}

	void Window::SwapBuffers()
	{
		m_SwapChain->Present();
	}

	void Window::SetVSync(bool enabled)
	{
		m_Specification.VSync = enabled;

		//TODO: hook into application
		m_SwapChain->SetVSync(m_Specification.VSync);
		m_SwapChain->OnResize(m_Specification.Width, m_Specification.Height);
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

	VulkanSwapChain& Window::GetSwapChain()
	{
		return *m_SwapChain;
	}

}

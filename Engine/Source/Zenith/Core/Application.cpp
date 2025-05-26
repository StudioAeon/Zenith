#include "znpch.hpp"
#include "Application.hpp"

extern bool g_ApplicationRunning;
namespace Zenith {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
	{
		s_Instance = this;

		m_EventBus.Listen<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
		m_EventBus.Listen<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
		m_EventBus.Listen<WindowMinimizeEvent>([this](WindowMinimizeEvent& e) { return OnWindowMinimize(e); });

		WindowSpecification windowSpec;
		windowSpec.Title = specification.Name;
		windowSpec.Width = specification.WindowWidth;
		windowSpec.Height = specification.WindowHeight;
		windowSpec.Fullscreen = specification.Fullscreen;
		windowSpec.VSync = specification.VSync;
		m_Window = std::unique_ptr<Window>(Window::Create(windowSpec));
		m_Window->Init();
		m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });

		if (specification.StartMaximized)
			m_Window->Maximize();
		else
			m_Window->CenterWindow();
		m_Window->SetResizable(specification.Resizable);
	}

	Application::~Application()
	{
		m_Window->SetEventCallback([](Event& e) {});
	}

	void Application::Run()
	{
		OnInit();
		while (m_Running)
		{
			ProcessEvents();

			if (!m_Minimized)
			{
				m_Window->SwapBuffers();
			}
		}
		
		OnShutdown();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnShutdown()
	{
		m_EventCallbacks.clear();
		g_ApplicationRunning = false;
	}

	void Application::ProcessEvents()
	{
		m_Window->ProcessEvents();
	}

	void Application::OnEvent(Event& event)
	{
		m_EventBus.Dispatch(event);

		if (!event.Handled)
		{
				for (auto& callback : m_EventCallbacks)
				{
						callback(event);
						if (event.Handled)
								break;
				}
		}
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		const uint32_t width = e.GetWidth(), height = e.GetHeight();
		if (width == 0 || height == 0)
		{
			return false;
		}

		return false;
	}

	bool Application::OnWindowMinimize(WindowMinimizeEvent& e)
	{
		m_Minimized = e.IsMinimized();
		return false;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		Close();
		return false;
	}

	const char* Application::GetConfigurationName()
	{
		return ZN_BUILD_CONFIG_NAME;
	}

	const char* Application::GetPlatformName()
	{
		return ZN_BUILD_PLATFORM_NAME;
	}

}
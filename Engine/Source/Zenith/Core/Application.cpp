#include "znpch.hpp"
#include "Application.hpp"

extern bool g_ApplicationRunning;
namespace Zenith {

	Application* Application::s_Instance = nullptr;

	Application::Application(int width, int height, const std::string& title)
	{
		s_Instance = this;

		m_Window = std::make_unique<Window>(title, width, height);
		
		AddEventCallback([this](Event& e) {
			return OnWindowClose(static_cast<WindowCloseEvent&>(e));
		});
		AddEventCallback([this](Event& e) {
			return OnWindowResize(static_cast<WindowResizeEvent&>(e));
		});
	}

	Application::~Application()
	{}

	void Application::Run()
	{
		OnInit();
		while (m_Running)
		{
			ProcessEvents();

			if (!m_Minimized)
			{
				OnUpdate();
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
		m_Window->PollEvents();

		if (m_Window->ShouldClose())
		{
			WindowCloseEvent e;
			OnEvent(e);
		}
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });

		if (event.Handled)
			return;

		for (auto& eventCallback : m_EventCallbacks)
		{
			eventCallback(event);

			if (event.Handled)
				break;
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
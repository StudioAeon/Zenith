#include "znpch.hpp"
#include "ApplicationContext.hpp"
#include "Application.hpp"

namespace Zenith {

	ApplicationContext::ApplicationContext(Application& app)
		: m_Application(app)
	{}

	Window& ApplicationContext::GetWindow()
	{
		return m_Application.GetWindow();
	}

	const Window& ApplicationContext::GetWindow() const
	{
		return m_Application.GetWindow();
	}

	EventBus& ApplicationContext::GetEventBus()
	{
		return m_Application.GetEventBus();
	}

	const EventBus& ApplicationContext::GetEventBus() const
	{
		return m_Application.GetEventBus();
	}

	const ApplicationSpecification& ApplicationContext::GetSpecification() const
	{
		return m_Application.GetSpecification();
	}

	Timestep ApplicationContext::GetTimestep() const
	{
		return m_Application.GetTimestep();
	}

	Timestep ApplicationContext::GetFrametime() const
	{
		return m_Application.GetFrametime();
	}

	PerformanceProfiler* ApplicationContext::GetPerformanceProfiler()
	{
		return m_Application.GetPerformanceProfiler();
	}

	ImGuiLayer* ApplicationContext::GetImGuiLayer()
	{
		return m_Application.GetImGuiLayer();
	}

	bool ApplicationContext::IsMainThread() const
	{
		return Application::IsMainThread();
	}

	std::unique_ptr<ApplicationContext> CreateApplicationContext(Application& app)
	{
		return std::make_unique<ApplicationContext>(app);
	}

}
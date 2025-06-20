#pragma once

#include "Zenith/Core/Base.hpp"

namespace Zenith {

	class Application;
	class Window;
	class EventBus;
	struct ApplicationSpecification;
	class PerformanceProfiler;
	class ImGuiLayer;
	class Timestep;

	class ApplicationContext
	{
	public:
		explicit ApplicationContext(Application& app);
		~ApplicationContext() = default;

		ApplicationContext(const ApplicationContext&) = delete;
		ApplicationContext& operator=(const ApplicationContext&) = delete;
		ApplicationContext(ApplicationContext&&) = default;
		ApplicationContext& operator=(ApplicationContext&&) = default;

		Window& GetWindow();
		const Window& GetWindow() const;

		EventBus& GetEventBus();
		const EventBus& GetEventBus() const;

		const ApplicationSpecification& GetSpecification() const;

		Timestep GetTimestep() const;
		Timestep GetFrametime() const;

		PerformanceProfiler* GetPerformanceProfiler();
		ImGuiLayer* GetImGuiLayer();

		bool IsMainThread() const;

	private:
		Application& m_Application;
	};

	std::unique_ptr<ApplicationContext> CreateApplicationContext(Application& app);

}
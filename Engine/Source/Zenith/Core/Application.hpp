#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Core/Window.hpp"

#include "Zenith/Events/ApplicationEvent.hpp"

namespace Zenith {

	struct ApplicationSpecification
	{
		std::string Name = "Zenith";
		uint32_t WindowWidth = 1920, WindowHeight = 1080;
		bool Fullscreen = false;
		bool VSync = true;
		bool StartMaximized = true;
		bool Resizable = true;
	};

	class Application
	{
		using EventCallbackFn = std::function<void(Event&)>;
	public:
		Application(const ApplicationSpecification& specification);
		virtual ~Application();

		void Run();
		void Close();

		virtual void OnInit() {}
		virtual void OnShutdown();
		virtual void OnUpdate() {}

		virtual void OnEvent(Event& event);

		void AddEventCallback(const EventCallbackFn& eventCallback) { m_EventCallbacks.push_back(eventCallback); }

		inline Window& GetWindow() { return *m_Window; }

		static inline Application& Get() { return *s_Instance; }

		static const char* GetConfigurationName();
		static const char* GetPlatformName();

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }
	private:
		void ProcessEvents();

		bool OnWindowResize(WindowResizeEvent& e);
		bool OnWindowMinimize(WindowMinimizeEvent& e);
		bool OnWindowClose(WindowCloseEvent& e);
	private:
		std::unique_ptr<Window> m_Window;
		ApplicationSpecification m_Specification;
		bool m_Running = true, m_Minimized = false;

		EventBus m_EventBus;
		std::vector<EventCallbackFn> m_EventCallbacks;

		float m_LastFrameTime = 0.0f;

		static Application* s_Instance;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}
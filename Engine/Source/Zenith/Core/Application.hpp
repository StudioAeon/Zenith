#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Core/Window.hpp"

#include "Zenith/Events/ApplicationEvent.hpp"

namespace Zenith {

	class Application
	{
		using EventCallbackFn = std::function<void(Event&)>;
	public:
		Application(int width = 1280, int height = 720, const std::string& title = "Zenith Engine");
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
	private:
		void ProcessEvents();

		bool OnWindowResize(WindowResizeEvent& e);
		bool OnWindowClose(WindowCloseEvent& e);
	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true, m_Minimized = false;

		std::vector<EventCallbackFn> m_EventCallbacks;

		float m_LastFrameTime = 0.0f;

		static Application* s_Instance;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}
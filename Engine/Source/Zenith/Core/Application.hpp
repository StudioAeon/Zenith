#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Core/TimeStep.hpp"
#include "Zenith/Core/Timer.hpp"
#include "Zenith/Core/Window.hpp"
#include "Zenith/Core/LayerStack.hpp"

#include "Zenith/Events/ApplicationEvent.hpp"
#include "Zenith/Events/KeyEvent.hpp"
#include "Zenith/Events/MouseEvent.hpp"

#include "Zenith/Editor/ImGui/ImGuiLayer.hpp"

namespace Zenith {

	struct ApplicationSpecification
	{
		std::string Name = "Zenith";
		uint32_t WindowWidth = 1920, WindowHeight = 1080;
		bool Fullscreen = false;
		bool VSync = true;
		bool StartMaximized = true;
		bool Resizable = true;
		bool EnableImGui = true;
		std::filesystem::path IconPath;
		std::string WorkingDirectory;
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
		virtual void OnUpdate(Timestep ts) {}

		virtual void OnEvent(Event& event);

		void PushLayer(const std::shared_ptr<Layer>& layer);
		void PushOverlay(const std::shared_ptr<Layer>& overlay);
		void PopLayer(const std::shared_ptr<Layer>& layer);
		void PopOverlay(const std::shared_ptr<Layer>& overlay);
		void RenderImGui();

		void AddEventCallback(const EventCallbackFn& eventCallback) { m_EventCallbacks.push_back(eventCallback); }
		EventBus& GetEventBus() { return m_EventBus; }

		inline Window& GetWindow() { return *m_Window; }

		static inline Application& Get() { return *s_Instance; }

		Timestep GetTimestep() const { return m_TimeStep; }
		Timestep GetFrametime() const { return m_Frametime; }

		float GetFrameDelta();  // TODO: This should be in "Platform"

		static const char* GetConfigurationName();
		static const char* GetPlatformName();

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		PerformanceProfiler* GetPerformanceProfiler() { return m_Profiler; }

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer.get(); }
	private:
		void ProcessEvents();
		void RegisterEventListeners();

		bool OnWindowResize(WindowResizeEvent& e);
		bool OnWindowMinimize(WindowMinimizeEvent& e);
		bool OnWindowClose(WindowCloseEvent& e);
	private:
		std::unique_ptr<Window> m_Window;
		ApplicationSpecification m_Specification;
		bool m_Running = true, m_Minimized = false;
		LayerStack m_LayerStack;
		std::shared_ptr<ImGuiLayer> m_ImGuiLayer;
		Timestep m_Frametime;
		Timestep m_TimeStep;
		PerformanceProfiler* m_Profiler = nullptr; // TODO: Should be null in Dist

		EventBus m_EventBus;
		std::vector<EventCallbackFn> m_EventCallbacks;

		float m_LastFrameTime = 0.0f;

		static Application* s_Instance;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}
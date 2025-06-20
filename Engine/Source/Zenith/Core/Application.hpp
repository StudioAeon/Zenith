#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Core/TimeStep.hpp"
#include "Zenith/Core/Timer.hpp"
#include "Zenith/Core/Window.hpp"
#include "Zenith/Core/LayerStack.hpp"

#include "Zenith/Events/ApplicationEvent.hpp"
#include "Zenith/ImGui/ImGuiLayer.hpp"
#include "Zenith/Renderer/RenderSystem.hpp"
#include "Zenith/Renderer/RenderThread.hpp"

namespace Zenith {

	class ApplicationContext;

	struct ApplicationSpecification
	{
		std::string Name = "Zenith";
		uint32_t WindowWidth = 1920, WindowHeight = 1080;
		bool Fullscreen = false;
		bool VSync = true;
		bool StartMaximized = true;
		bool Resizable = true;
		bool EnableImGui = true;
		bool ShowSplashScreen = true;
		ThreadingPolicy CoreThreadingPolicy = ThreadingPolicy::SingleThreaded;
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

		EventBus& GetEventBus() { return m_EventBus; }
		Window& GetWindow() { return *m_Window; }
		LayerStack& GetLayerStack() { return m_LayerStack; }

		Timestep GetTimestep() const { return m_TimeStep; }
		Timestep GetFrametime() const { return m_Frametime; }
		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		PerformanceProfiler* GetPerformanceProfiler() { return m_Profiler; }
		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer.get(); }

		std::shared_ptr<ApplicationContext> GetApplicationContext() { return m_ApplicationContext; }

		float GetFrameDelta();

		static std::thread::id GetMainThreadID();
		static bool IsMainThread();

		static const char* GetConfigurationName();
		static const char* GetPlatformName();

	private:
		void ProcessEvents();
		void RegisterEventListeners();

		bool OnWindowResize(WindowResizeEvent& e);
		bool OnWindowMinimize(WindowMinimizeEvent& e);
		bool OnWindowClose(WindowCloseEvent& e);
	private:
		std::unique_ptr<Window> m_Window;
		ApplicationSpecification m_Specification;
		LayerStack m_LayerStack;
		EventBus m_EventBus;
		RenderSystem m_RenderSystem;

		bool m_Running = true, m_Minimized = false;
		Timestep m_Frametime;
		Timestep m_TimeStep;
		float m_LastFrameTime = 0.0f;

		std::shared_ptr<ImGuiLayer> m_ImGuiLayer;
		PerformanceProfiler* m_Profiler = nullptr; // TODO: Should be null in Dist

		std::shared_ptr<ApplicationContext> m_ApplicationContext;

		static std::thread::id s_MainThreadID;

	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}

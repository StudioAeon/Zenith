#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Core/TimeStep.hpp"
#include "Zenith/Core/Timer.hpp"
#include "Zenith/Core/Window.hpp"
#include "Zenith/Core/LayerStack.hpp"
#include "Zenith/Renderer/RendererConfig.hpp"

#include "Zenith/Events/ApplicationEvent.hpp"
#include "Zenith/ImGui/ImGuiLayer.hpp"
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
		RendererConfig RenderConfig;
		ThreadingPolicy CoreThreadingPolicy = ThreadingPolicy::MultiThreaded;
		std::filesystem::path IconPath;
		std::string WorkingDirectory;
	};

	class Application
	{
		using EventCallbackFn = std::function<void(Event&)>;
	public:
		struct PerformanceTimers
		{
			float MainThreadWorkTime = 0.0f;
			float MainThreadWaitTime = 0.0f;
			float RenderThreadWorkTime = 0.0f;
			float RenderThreadWaitTime = 0.0f;
			float RenderThreadGPUWaitTime = 0.0f;
		};
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
		Window& GetWindow()
		{
			ZN_CORE_ASSERT(m_Window, "Window is null!");
			return *m_Window;
		}
		LayerStack& GetLayerStack() { return m_LayerStack; }

		Timestep GetTimestep() const { return m_TimeStep; }
		Timestep GetFrametime() const { return m_Frametime; }
		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		PerformanceProfiler* GetPerformanceProfiler() { return m_Profiler; }
		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer.get(); }

		RenderThread& GetRenderThread() { return m_RenderThread; }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		const PerformanceTimers& GetPerformanceTimers() const { return m_PerformanceTimers; }
		PerformanceTimers& GetPerformanceTimers() { return m_PerformanceTimers; }
		const std::unordered_map<const char*, PerformanceProfiler::PerFrameData>& GetProfilerPreviousFrameData() const { return m_ProfilerPreviousFrameData; }

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

		bool m_Running = true, m_Minimized = false;
		Timestep m_Frametime;
		Timestep m_TimeStep;
		float m_LastFrameTime = 0.0f;
		uint32_t m_CurrentFrameIndex = 0;

		PerformanceTimers m_PerformanceTimers; // TODO: remove for Dist

		std::shared_ptr<ImGuiLayer> m_ImGuiLayer;
		PerformanceProfiler* m_Profiler = nullptr; // TODO: Should be null in Dist
		std::unordered_map<const char*, PerformanceProfiler::PerFrameData> m_ProfilerPreviousFrameData;

		RenderThread m_RenderThread;

		std::shared_ptr<ApplicationContext> m_ApplicationContext;

		static std::thread::id s_MainThreadID;

	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}

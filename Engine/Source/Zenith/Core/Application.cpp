#include "znpch.hpp"
#include "Application.hpp"

#include "Zenith/Renderer/API/Renderer.hpp"

#include <SDL3/SDL.h>

#include "Input.hpp"
#include "FatalSignal.hpp"

#include "Zenith/Debug/Profiler.hpp"

#include <glm/glm.hpp>

bool g_ApplicationRunning = true;
namespace Zenith {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
	{
		FatalSignal::Install(3000);

		FatalSignal::AddCallback([this]() {
			if (m_Window)
				m_Window->SetEventCallback([](Event&) {});
			Renderer::Shutdown();
		});

		s_Instance = this;

		RegisterEventListeners();

		m_Profiler = znew PerformanceProfiler();

		WindowSpecification windowSpec;
		windowSpec.Title = specification.Name;
		windowSpec.Width = specification.WindowWidth;
		windowSpec.Height = specification.WindowHeight;
		windowSpec.Fullscreen = specification.Fullscreen;
		windowSpec.VSync = specification.VSync;
		windowSpec.IconPath = specification.IconPath;
		m_Window = std::unique_ptr<Window>(Window::Create(windowSpec));
		m_Window->Init();

		m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });

		Renderer::Init();
		Renderer::WaitAndRender();

		if (specification.StartMaximized)
			m_Window->Maximize();
		else
			m_Window->CenterWindow();
		m_Window->SetResizable(specification.Resizable);
	}

	Application::~Application()
	{
		m_Window->SetEventCallback([](Event& e) {});

		for (const auto& layer : m_LayerStack)
		{
			if (layer->IsEnabled())
				layer->OnDetach();
		}
		m_LayerStack.Clear();

		Renderer::Shutdown();

		delete m_Profiler;
		m_Profiler = nullptr;
	}

	void Application::PushLayer(const std::shared_ptr<Layer>& layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(const std::shared_ptr<Layer>& overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::PopLayer(const std::shared_ptr<Layer>& layer)
	{
		m_LayerStack.PopLayer(layer);
		layer->OnDetach();
	}

	void Application::PopOverlay(const std::shared_ptr<Layer>& overlay)
	{
		m_LayerStack.PopOverlay(overlay);
		overlay->OnDetach();
	}

	void Application::Run()
	{
		OnInit();
		while (m_Running)
		{
			m_Frametime = GetFrameDelta();
			m_TimeStep = glm::min<float>(m_Frametime, 0.0333f);
			m_LastFrameTime += m_Frametime; // Keep total time

			ProcessEvents();

			m_Profiler->Clear();

			if (!m_Minimized)
			{
				Renderer::BeginFrame();
				{
					ZN_SCOPE_PERF("Application Layer::OnUpdate");
					for (const auto& layer : m_LayerStack)
						if (layer->IsEnabled())
							layer->OnUpdate(m_TimeStep);
				}
				Renderer::EndFrame();

				m_Window->GetRenderContext()->BeginFrame();
				Renderer::WaitAndRender();
				m_Window->SwapBuffers();

				Renderer::SwapQueues(); // TODO: Should be render thread later
			}

			Input::ClearReleasedKeys();
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
		Input::TransitionPressedKeys();
		Input::TransitionPressedButtons();

		m_Window->ProcessEvents();

		m_EventBus.DispatchQueued();
	}

	void Application::RegisterEventListeners()
	{
		// Window Events
		m_EventBus.Listen<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
		m_EventBus.Listen<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
		m_EventBus.Listen<WindowMinimizeEvent>([this](WindowMinimizeEvent& e) { return OnWindowMinimize(e); });

		// Keyboard Events
		m_EventBus.Listen<KeyPressedEvent>([this](KeyPressedEvent& e) {
			return false;
		});
		m_EventBus.Listen<KeyReleasedEvent>([this](KeyReleasedEvent& e) {
			return false;
		});
		m_EventBus.Listen<KeyTypedEvent>([this](KeyTypedEvent& e) {
			return false;
		});

		// Mouse Events
		m_EventBus.Listen<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e) {
			return false;
		});
		m_EventBus.Listen<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& e) {
			return false;
		});
		m_EventBus.Listen<MouseMovedEvent>([this](MouseMovedEvent& e) {
			return false;
		});
		m_EventBus.Listen<MouseScrolledEvent>([this](MouseScrolledEvent& e) {
			return false;
		});
	}


	void Application::OnEvent(Event& event)
	{
		m_EventBus.Dispatch(event);

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); ) {
			(*--it)->OnEvent(event);
			if (event.Handled || event.IsPropagationStopped())
				break;
		}

		if (event.Handled || event.IsPropagationStopped())
			return;

		for (auto& eventCallback : m_EventCallbacks) {
			eventCallback(event);
			if (event.Handled || event.IsPropagationStopped())
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

		m_Window->GetRenderContext()->OnResize(width, height);
		return true;
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

	float Application::GetFrameDelta()
	{
		static uint64_t last = SDL_GetTicksNS();
		uint64_t now = SDL_GetTicksNS();
		uint64_t delta = now - last;
		last = now;
		return static_cast<float>(delta * 1e-9f);
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
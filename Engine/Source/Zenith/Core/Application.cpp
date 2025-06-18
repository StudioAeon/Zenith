#include "znpch.hpp"
#include "Application.hpp"
#include "SplashScreen.hpp"

#include "Zenith/Renderer/Renderer.hpp"
#include "Zenith/Renderer/Font.hpp"

// #include "Zenith/Audio/AudioEngine.hpp"

#include "Zenith/Events/KeyEvent.hpp"
#include "Zenith/Events/MouseEvent.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

#include "Input.hpp"
#include "FatalSignal.hpp"

#include <imgui_internal.h>

#include "Zenith/Utilities/StringUtils.hpp"
#include "Zenith/Debug/Profiler.hpp"

#include <filesystem>
#include <nfd.hpp>

#include "Memory.hpp"

bool g_ApplicationRunning = true;
extern ImGuiContext* GImGui;
namespace Zenith {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		FatalSignal::Install(3000);

		FatalSignal::AddCallback([this]() {
			if (m_Window)
				m_Window->SetEventCallback([](Event&) {});
			Renderer::Shutdown();
		});

		s_Instance = this;

		if (!specification.WorkingDirectory.empty())
			std::filesystem::current_path(specification.WorkingDirectory);

		m_Profiler = znew PerformanceProfiler();

		bool showSplash = specification.ShowSplashScreen;
		if (showSplash)
		{
			SplashScreen::Config splashConfig;
			splashConfig.ImagePath = "Resources/Editor/Zenith_Splash.png";
			splashConfig.WindowWidth = 448;
			splashConfig.WindowHeight = 448;
			splashConfig.DisplayTime = 1.8f;
			splashConfig.AllowSkip = true;
			splashConfig.BackgroundColor = { 20, 20, 25, 255 };

			auto splash = std::make_unique<SplashScreen>(splashConfig);
			if (splash->Initialize())
			{
				splash->Show();
			}
		}

		RegisterEventListeners();

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

		ZN_CORE_VERIFY(NFD::Init() == NFD_OKAY);

		Renderer::Init();
		Renderer::WaitAndRender();

		if (specification.StartMaximized)
			m_Window->Maximize();
		else
			m_Window->CenterWindow();
		m_Window->SetResizable(specification.Resizable);

		if (m_Specification.EnableImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create();
			PushOverlay(m_ImGuiLayer);
		}

		// AudioEngine::Init();
		Font::Init();
	}

	Application::~Application()
	{
		NFD::Quit();

		m_Window->SetEventCallback([](Event& e) {});

		for (const auto& layer : m_LayerStack)
		{
			if (layer->IsEnabled())
				layer->OnDetach();
		}
		m_LayerStack.Clear();

		// AudioEngine::Shutdown();
		Font::Shutdown();

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

	void Application::RenderImGui()
	{
		ZN_PROFILE_FUNC();
		ZN_SCOPE_PERF("Application::RenderImGui");

		m_ImGuiLayer->Begin();

		ImGui::Begin("Renderer");
		auto& caps = Renderer::GetCapabilities();
		ImGui::Text("Vendor: %s", caps.Vendor.c_str());
		ImGui::Text("Renderer: %s", caps.Device.c_str());
		ImGui::Text("Version: %s", caps.Version.c_str());
		ImGui::Text("Frame Time: %.2fms\n", m_TimeStep.GetMilliseconds());
		ImGui::End();

		for (int i = 0; i < m_LayerStack.Size(); i++)
			m_LayerStack[i]->OnImGuiRender();
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

				// Render ImGui on render thread
				Application* app = this;
				if (m_Specification.EnableImGui)
				{
					Renderer::Submit([app]() { app->RenderImGui(); });
					Renderer::Submit([this]() { m_ImGuiLayer->End(); });
				}
				Renderer::EndFrame();

				// TODO: Clean up this frame render flow
				m_Window->GetRenderContext()->BeginFrame();
				Renderer::WaitAndRender();

				Renderer::Submit([&]()
				{
					m_Window->SwapBuffers();
				});

				Renderer::SwapQueues();
			}

			Input::ClearReleasedKeys();

			ZN_PROFILE_MARK_FRAME;
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
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;

		Renderer::WaitAndRender();
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
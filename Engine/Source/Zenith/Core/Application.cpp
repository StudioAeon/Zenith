#include "znpch.hpp"
#include "Application.hpp"
#include "ApplicationContext.hpp"
#include "SplashScreen.hpp"

#include "Zenith/Renderer/Renderer.hpp"
#include "Zenith/Renderer/Framebuffer.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

// #include "Zenith/Audio/AudioEngine.hpp"

#include "Zenith/Events/KeyEvent.hpp"
#include "Zenith/Events/MouseEvent.hpp"

#include "Input.hpp"
#include "FatalSignal.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanRenderer.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanAllocator.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanSwapChain.hpp"
#include <imgui_internal.h>

#include "Zenith/Utilities/StringUtils.hpp"
#include "Zenith/Debug/Profiler.hpp"

#include <filesystem>
#include <nfd.hpp>

#include "Memory.hpp"
#include "backends/imgui_impl_vulkan.h"

bool g_ApplicationRunning = true;
extern ImGuiContext* GImGui;

namespace Zenith {

	std::thread::id Application::s_MainThreadID;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification), m_RenderThread(specification.CoreThreadingPolicy)
	{
		FatalSignal::Install(3000);

		/*FatalSignal::AddCallback([this]() {
			if (m_Window)
				m_Window->SetEventCallback([](Event&) {});
			Renderer::Shutdown();
		});*/

		s_MainThreadID = std::this_thread::get_id();

		if (!specification.WorkingDirectory.empty())
			std::filesystem::current_path(specification.WorkingDirectory);

		m_Profiler = znew PerformanceProfiler();

		Renderer::SetConfig(specification.RenderConfig);

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
			splash->Show();
		}

		WindowSpecification windowSpec;
		windowSpec.Title = m_Specification.Name;
		windowSpec.Width = m_Specification.WindowWidth;
		windowSpec.Height = m_Specification.WindowHeight;
		windowSpec.Fullscreen = m_Specification.Fullscreen;
		windowSpec.VSync = m_Specification.VSync;
		windowSpec.Maximized = m_Specification.StartMaximized;
		windowSpec.Resizable = m_Specification.Resizable;

		if (!m_Specification.IconPath.empty())
			windowSpec.IconPath = m_Specification.IconPath;

		m_Window = Window::Create(windowSpec, this);
		m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });

		m_ApplicationContext = std::make_shared<ApplicationContext>(*this);

		RegisterEventListeners();

		ZN_CORE_VERIFY(NFD::Init() == NFD_OKAY);

		m_RenderThread.Run();
		Renderer::Init(this);

		if (m_Specification.EnableImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create(*m_ApplicationContext);
			PushOverlay(m_ImGuiLayer);
		}

		// Render one frame (TODO: maybe make a func called Pump or something)
		m_RenderThread.Pump();
	}

	Application::~Application()
	{
		NFD::Quit();

		m_Window->SetEventCallback([](Event& e) {});

		if (m_RenderThread.IsRunning()) {
			m_RenderThread.BlockUntilRenderComplete();
		}


		m_RenderThread.Terminate();

		Renderer::Shutdown();

		delete m_Profiler;
		m_Profiler = nullptr;
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnShutdown()
	{
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
		m_EventBus.Listen<KeyPressedEvent>([this](KeyPressedEvent& e) { return false; });
		m_EventBus.Listen<KeyReleasedEvent>([this](KeyReleasedEvent& e) { return false; });
		m_EventBus.Listen<KeyTypedEvent>([this](KeyTypedEvent& e) { return false; });

		// Mouse Events
		m_EventBus.Listen<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e) { return false; });
		m_EventBus.Listen<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& e) { return false; });
		m_EventBus.Listen<MouseMovedEvent>([this](MouseMovedEvent& e) { return false; });
		m_EventBus.Listen<MouseScrolledEvent>([this](MouseScrolledEvent& e) { return false; });
	}

	void Application::OnEvent(Event& event)
	{
		m_EventBus.Dispatch(event);

		if (event.Handled || event.IsPropagationStopped())
			return;

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); ) {
			--it;

			if (!(*it)->IsEnabled())
				continue;

			bool handled = (*it)->OnEvent(event);
			if (handled) {
				event.Handled = true;
			}

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

		auto& window = m_Window;
		Renderer::Submit([&window, width, height]() mutable
		{
			window->GetSwapChain().OnResize(width, height);
		});

		return false;
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

			// Wait for render thread to finish frame
			{
				ZN_PROFILE_SCOPE("Wait");
				Timer timer;

				m_RenderThread.BlockUntilRenderComplete();

				m_PerformanceTimers.MainThreadWaitTime = timer.ElapsedMillis();
			}

			static uint64_t frameCounter = 0;

			ProcessEvents(); // Poll events when both threads are idle

			m_ProfilerPreviousFrameData = m_Profiler->GetPerFrameData();
			m_Profiler->Clear();

			m_RenderThread.NextFrame();

			// Start rendering previous frame
			m_RenderThread.Kick();

			if (!m_Minimized)
			{
				Timer cpuTimer;

				// On Render thread
				Renderer::Submit([&]()
				{
					m_Window->GetSwapChain().BeginFrame();
				});

				Renderer::BeginFrame();
				{
					ZN_SCOPE_PERF("Application Layer::OnUpdate");
					for (std::shared_ptr<Layer>& layer : m_LayerStack)
						layer->OnUpdate(m_TimeStep);
				}


				// Render ImGui on render thread
				Application* app = this;
				if (m_Specification.EnableImGui)
				{
					Renderer::Submit([app]() { app->RenderImGui(); });
					Renderer::Submit([=]() { m_ImGuiLayer->End(); });
				}
				Renderer::EndFrame();

				// On Render thread
				Renderer::Submit([&]()
				{
					m_Window->SwapBuffers();
				});

				m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Renderer::GetConfig().FramesInFlight;
				m_PerformanceTimers.MainThreadWorkTime = cpuTimer.ElapsedMillis();
			}
			Input::ClearReleasedKeys();

			frameCounter++;

			ZN_PROFILE_MARK_FRAME;
		}
		OnShutdown();
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

	std::thread::id Application::GetMainThreadID() { return s_MainThreadID; }

	bool Application::IsMainThread()
	{
		return std::this_thread::get_id() == s_MainThreadID;
	}

}
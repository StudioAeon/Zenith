#include "znpch.hpp"
#include "OpenGLImGuiLayer.hpp"

#include <imgui/imgui.h>
#include <glad/glad.h>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include "Zenith/Core/ApplicationContext.hpp"
#include "Zenith/Core/Window.hpp"
#include <SDL3/SDL.h>

#include "Zenith/Renderer/Renderer.hpp"

namespace Zenith {

	OpenGLImGuiLayer::OpenGLImGuiLayer(ApplicationContext& context)
		: ImGuiLayer(context)
	{}

	OpenGLImGuiLayer::~OpenGLImGuiLayer()
	{}

	void OpenGLImGuiLayer::OnAttach()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

		ImFont* pFont = io.Fonts->AddFontFromFileTTF("Resources/Fonts/Roboto/Roboto-Regular.ttf", 18.0f);
		io.FontDefault = io.Fonts->Fonts.back();

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

		Window& window = GetContext().GetWindow();
		SDL_Window* sdlWindow = static_cast<SDL_Window*>(window.GetNativeWindow());

		if (!sdlWindow) {
			ZN_CORE_ERROR("SDL Window is null!");
			return;
		}

		SDL_GLContext gl_context = SDL_GL_GetCurrentContext();
		if (!gl_context) {
			ZN_CORE_ERROR("No OpenGL context!");
			return;
		}

		// Setup Platform/Renderer bindings
		ImGui_ImplSDL3_InitForOpenGL(sdlWindow, gl_context);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void OpenGLImGuiLayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}

	void OpenGLImGuiLayer::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
	}

	void OpenGLImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();

		Window& window = GetContext().GetWindow();
		io.DisplaySize = ImVec2(static_cast<float>(window.GetWidth()), static_cast<float>(window.GetHeight()));

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
			SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
		}
	}

}
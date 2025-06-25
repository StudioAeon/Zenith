#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Events/Event.hpp"
#include "Zenith/Renderer/RendererContext.hpp"
#include "Zenith/Renderer/RendererAPI.hpp"

#include <filesystem>

#include <SDL3/SDL.h>

// #include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"

namespace Zenith {

	struct WindowSpecification
	{
		std::string Title = "Zenith";
		uint32_t Width = 1600;
		uint32_t Height = 900;
		bool Fullscreen = false;
		bool VSync = true;
		bool Maximized = false;
		bool Resizable = true;
		std::filesystem::path IconPath;
	};

	class VulkanSwapChain;

	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		Window(const WindowSpecification& specification);
		~Window();

		virtual void Init();
		virtual void ProcessEvents();
		virtual void SwapBuffers();

		inline uint32_t GetWidth() const { return m_Data.Width; }
		inline uint32_t GetHeight() const { return m_Data.Height; }

		virtual std::pair<uint32_t, uint32_t> GetSize() const { return { m_Data.Width, m_Data.Height }; }
		virtual std::pair<float, float> GetWindowPos() const;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
		virtual void SetVSync(bool enabled);
		virtual bool IsVSync() const;
		virtual void SetResizable(bool resizable) const;

		virtual void Maximize();
		virtual void CenterWindow();

		virtual const std::string& GetTitle() const { return m_Data.Title; }
		virtual void SetTitle(const std::string& title);

		inline void* GetNativeWindow() const { return m_Window; }

		void SetRenderContext(Ref<RendererContext> context) { m_RendererContext = context; }

		virtual Ref<RendererContext> GetRenderContext() { return m_RendererContext; }
		virtual VulkanSwapChain& GetSwapChain();
	public:
		static std::unique_ptr<Window> Create(const WindowSpecification& specification = WindowSpecification());

	private:
		virtual void Shutdown();
		virtual void PollEvents();
	private:
		SDL_Window* m_Window = nullptr;
		SDL_Event m_Event{};

		WindowSpecification m_Specification;
		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
		float m_LastFrameTime = 0.0f;

		Ref<RendererContext> m_RendererContext;
		VulkanSwapChain* m_SwapChain;
	};

}

#pragma once

#include "Zenith/Renderer/API/RendererContext.hpp"

#include <SDL3/SDL.h>

namespace Zenith {

	class OpenGLContext : public RendererContext
	{
	public:
		OpenGLContext(SDL_Window* windowHandle);
		virtual ~OpenGLContext();

		virtual void Create() override;
		virtual void BeginFrame() override {}
		virtual void SwapBuffers() override;
		virtual void OnResize(uint32_t width, uint32_t height) override {}
	private:
		SDL_Window* m_WindowHandle;
		SDL_GLContext m_GLContext;
	};

}

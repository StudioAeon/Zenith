#include "znpch.hpp"
#include "OpenGLContext.hpp"

#include <glad/glad.h>

#include "Zenith/Core/Log.hpp"

namespace Zenith {

	OpenGLContext::OpenGLContext(SDL_Window* windowHandle)
		: m_WindowHandle(windowHandle)
	{}

	OpenGLContext::~OpenGLContext()
	{}

	void OpenGLContext::Create()
	{
		ZN_CORE_INFO_TAG("Renderer", "OpenGLContext::Create");

		m_GLContext = SDL_GL_CreateContext(m_WindowHandle);
		ZN_CORE_ASSERT(m_GLContext, "Failed to create OpenGL context!");

		SDL_GL_MakeCurrent(m_WindowHandle, m_GLContext);
		int status = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
		ZN_CORE_ASSERT(status, "Failed to initialize Glad!");

#ifdef ZN_ENABLE_ASSERTS
		int versionMajor;
		int versionMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

		ZN_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5), "Zenith requires at least OpenGL version 4.5!");
#endif
	}

	void OpenGLContext::SwapBuffers()
	{
		SDL_GL_SwapWindow(m_WindowHandle);
	}

}

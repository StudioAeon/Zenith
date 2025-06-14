#include "znpch.hpp"
#include "OpenGLRenderer.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include <glad/glad.h>

namespace Zenith {

	struct OpenGLRendererData
	{
		RendererCapabilities RenderCaps;
	};

	static OpenGLRendererData* s_Data = nullptr;

	namespace Utils {

		static void Clear(float r, float g, float b, float a)
		{
			glClearColor(r, g, b, a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		static void SetClearColor(float r, float g, float b, float a)
		{
			glClearColor(r, g, b, a);
		}

		static void DrawIndexed(uint32_t count)
		{
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(count), GL_UNSIGNED_INT, nullptr);
		}

		static void OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
		{
			switch (severity)
			{
				case GL_DEBUG_SEVERITY_HIGH:
					ZN_CORE_ERROR("[OpenGL Debug HIGH] {0}", message);
					ZN_CORE_ASSERT(false, "GL_DEBUG_SEVERITY_HIGH");
					break;
				case GL_DEBUG_SEVERITY_MEDIUM:
					ZN_CORE_WARN("[OpenGL Debug MEDIUM] {0}", message);
					break;
				case GL_DEBUG_SEVERITY_LOW:
					ZN_CORE_INFO("[OpenGL Debug LOW] {0}", message);
					break;
				case GL_DEBUG_SEVERITY_NOTIFICATION:
					// ZN_CORE_TRACE("[OpenGL Debug NOTIFICATION] {0}", message);
					break;
			}
		}

	}

	void OpenGLRenderer::Init()
	{
		s_Data = new OpenGLRendererData();
		Renderer::Submit([]()
		{
			glDebugMessageCallback(Utils::OpenGLLogMessage, nullptr);
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

			auto& caps = s_Data->RenderCaps;
			caps.Vendor = (const char*)glGetString(GL_VENDOR);
			caps.Device = (const char*)glGetString(GL_RENDERER);
			caps.Version = (const char*)glGetString(GL_VERSION);
			ZN_CORE_TRACE_TAG("Renderer", "OpenGLRendererData::Init");
			Utils::DumpGPUInfo();

			//unsigned int vao;
			//glGenVertexArrays(1, &vao);
			//glBindVertexArray(vao);

			glGetIntegerv(GL_MAX_SAMPLES, &caps.MaxSamples);
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.MaxAnisotropy);

			glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &caps.MaxTextureUnits);

			GLenum error = glGetError();
			while (error != GL_NO_ERROR)
			{
				ZN_CORE_ERROR_TAG("Renderer", "OpenGL Error {0}", error);
				error = glGetError();
			}
		});
	}

	void OpenGLRenderer::Shutdown()
	{
		delete s_Data;
	}

	RendererCapabilities& OpenGLRenderer::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}

	void OpenGLRenderer::BeginFrame()
	{
		Utils::Clear(0.1f, 0.1f, 0.1f, 1.0f);
	}

	void OpenGLRenderer::EndFrame()
	{}

}

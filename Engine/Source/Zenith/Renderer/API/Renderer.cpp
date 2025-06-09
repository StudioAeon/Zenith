#include "znpch.hpp"
#include "Renderer.hpp"

#include "RendererAPI.hpp"

#include "Zenith/Renderer/OpenGL/OpenGLRenderer.hpp"

namespace Zenith {

	static RendererAPI* s_RendererAPI = nullptr;

	void RendererAPI::SetAPI(RendererAPIType api)
	{
		// TODO: make sure this is called at a valid time
		ZN_CORE_ASSERT(api == RendererAPIType::OpenGL || api == RendererAPIType::Vulkan,
			"Only OpenGL and Vulkan are currently supported");
		s_CurrentRendererAPI = api;
	}

	constexpr static uint32_t s_RenderCommandQueueCount = 2;
	static RenderCommandQueue* s_CommandQueue[s_RenderCommandQueueCount];
	static std::atomic<uint32_t> s_RenderCommandQueueSubmissionIndex = 0;

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::OpenGL: return znew OpenGLRenderer();
			case RendererAPIType::None:
				ZN_CORE_ASSERT(false, "RendererAPI::None is not supported");
				return nullptr;
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void Renderer::Init()
	{
		s_CommandQueue[0] = znew RenderCommandQueue();
		s_CommandQueue[1] = znew RenderCommandQueue();

		s_RendererAPI = InitRendererAPI();

		Renderer::WaitAndRender();

		s_RendererAPI->Init();
	}

	void Renderer::Shutdown()
	{
		s_RendererAPI->Shutdown();

		delete s_CommandQueue[0];
		delete s_CommandQueue[1];
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	void Renderer::WaitAndRender()
	{
		s_CommandQueue[GetRenderQueueIndex()]->Execute();
	}

	void Renderer::SwapQueues()
	{
		s_RenderCommandQueueSubmissionIndex = (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	uint32_t Renderer::GetRenderQueueIndex()
	{
		return (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	uint32_t Renderer::GetRenderQueueSubmissionIndex()
	{
		return s_RenderCommandQueueSubmissionIndex;
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue[s_RenderCommandQueueSubmissionIndex];
	}

}

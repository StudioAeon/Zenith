#include "znpch.hpp"
#include "Renderer.hpp"

#include "RendererAPI.hpp"

#include "Zenith/Core/Timer.hpp"
#include "Zenith/Debug/Profiler.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanRenderer.hpp"
#include "Zenith/Project/Project.hpp"

#include <filesystem>
#include <format>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

namespace Zenith {
	Application* Renderer::s_Application = nullptr;
	static RendererAPI* s_RendererAPI = nullptr;

	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		// Swapchain owns the Render Thread frame index
		return s_Application->GetWindow().GetSwapChain().GetCurrentBufferIndex();
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return s_Application->GetCurrentFrameIndex();
	}

	void RendererAPI::SetAPI(RendererAPIType api)
	{
		// TODO: make sure this is called at a valid time
		ZN_CORE_VERIFY(api == RendererAPIType::Vulkan, "Vulkan is currently the only supported Renderer API");
		s_CurrentRendererAPI = api;
	}

	static RendererConfig s_Config;
	constexpr static uint32_t s_RenderCommandQueueCount = 2;
	static RenderCommandQueue* s_CommandQueue[s_RenderCommandQueueCount];
	static std::atomic<uint32_t> s_RenderCommandQueueSubmissionIndex = 0;
	static RenderCommandQueue s_ResourceFreeQueue[3];

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::Vulkan: return znew VulkanRenderer();
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void Renderer::Init(Application* app)
	{
		s_Application = app;
		s_CommandQueue[0] = znew RenderCommandQueue();
		s_CommandQueue[1] = znew RenderCommandQueue();

		Renderer::SetCurrentContext(app->GetWindow().GetRenderContext());

		s_RendererAPI = InitRendererAPI();
		s_RendererAPI->Init();

		// Now we can safely access the swap chain through the window
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight,
													 app->GetWindow().GetSwapChain().GetImageCount());
	}

	void Renderer::Shutdown()
	{
		s_RendererAPI->Shutdown();

		// Resource release queue
		for (uint32_t i = 0; i < s_Config.FramesInFlight; i++)
		{
			auto& queue = Renderer::GetRenderResourceReleaseQueue(i);
			queue.Execute();
		}

		delete s_CommandQueue[0];
		delete s_CommandQueue[1];
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	void Renderer::RenderThreadFunc(RenderThread* renderThread)
	{
		ZN_PROFILE_THREAD("Render Thread");

		while (renderThread->IsRunning())
		{
			WaitAndRender(); // Todo: WaitAndRender(RenderThread* renderThread)
		}
	}

	void Renderer::WaitAndRender()
	{
		ZN_PROFILE_FUNC();
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

	void Renderer::InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& color)
	{
		s_RendererAPI->InsertGPUPerfMarker(renderCommandBuffer, label, color);
	}

	void Renderer::BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor)
	{
		s_RendererAPI->BeginGPUPerfMarker(renderCommandBuffer, label, markerColor);
	}

	void Renderer::EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndGPUPerfMarker(renderCommandBuffer);
	}

	void Renderer::RT_InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& color)
	{
		s_RendererAPI->RT_InsertGPUPerfMarker(renderCommandBuffer, label, color);
	}

	void Renderer::RT_BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor)
	{
		s_RendererAPI->RT_BeginGPUPerfMarker(renderCommandBuffer, label, markerColor);
	}

	void Renderer::RT_EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->RT_EndGPUPerfMarker(renderCommandBuffer);
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void Renderer::ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange)
	{
		s_RendererAPI->ClearImage(renderCommandBuffer, image, clearValue, subresourceRange);
	}

	void Renderer::CopyImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->CopyImage(renderCommandBuffer, sourceImage, destinationImage);
	}

	void Renderer::BlitImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->BlitImage(renderCommandBuffer, sourceImage, destinationImage);
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue[s_RenderCommandQueueSubmissionIndex];
	}

	RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}

	RendererConfig& Renderer::GetConfig()
	{
		return s_Config;
	}

	void Renderer::SetConfig(const RendererConfig& config)
	{
		s_Config = config;
	}

	GPUMemoryStats Renderer::GetGPUMemoryStats()
	{
		return VulkanAllocator::GetStats();
	}

}

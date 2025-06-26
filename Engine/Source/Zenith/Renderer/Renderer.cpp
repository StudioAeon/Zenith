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
	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;
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

	struct RendererData
	{
		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
	};

	static RendererConfig s_Config;
	static RendererData* s_Data = nullptr;
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
		s_Data = znew RendererData();
		s_CommandQueue[0] = znew RenderCommandQueue();
		s_CommandQueue[1] = znew RenderCommandQueue();

		Renderer::SetCurrentContext(app->GetWindow().GetRenderContext());

		// Make sure we don't have more frames in flight than swapchain images
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight, app->GetWindow().GetSwapChain().GetImageCount());

		s_RendererAPI = InitRendererAPI();

		Renderer::GetApplication()->GetRenderThread().Pump();

		uint32_t whiteTextureData = 0xffffffff;
		TextureSpecification spec;
		spec.Format = ImageFormat::RGBA;
		spec.Width = 1;
		spec.Height = 1;
		s_Data->WhiteTexture = Texture2D::Create(spec, Buffer(&whiteTextureData, sizeof(uint32_t)));

		constexpr uint32_t blackTextureData = 0xff000000;
		s_Data->BlackTexture = Texture2D::Create(spec, Buffer(&blackTextureData, sizeof(uint32_t)));

		s_RendererAPI->Init();
	}

	void Renderer::Shutdown()
	{
		s_RendererAPI->Shutdown();

		delete s_Data;

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

		Renderer::SetCurrentContext(s_Application->GetWindow().GetRenderContext());

		while (renderThread->IsRunning())
		{
			WaitAndRender(renderThread);
		}
	}

	void Renderer::WaitAndRender(RenderThread* renderThread)
	{
		ZN_PROFILE_FUNC();

		if (!renderThread->IsRunning()) {
			return;
		}
		auto& performanceTimers = Renderer::s_Application->GetPerformanceTimers();

		// Wait for kick, then set render thread to busy
		{
			ZN_PROFILE_SCOPE("Wait");
			Timer waitTimer;
			renderThread->WaitAndSet(RenderThread::State::Kick, RenderThread::State::Busy);

			if (!renderThread->IsRunning()) {
				renderThread->Set(RenderThread::State::Idle);
				return;
			}

			performanceTimers.RenderThreadWaitTime = waitTimer.ElapsedMillis();
		}

		Timer workTimer;
		if (renderThread->IsRunning()) {
			s_CommandQueue[GetRenderQueueIndex()]->Execute();
		}

		// Rendering has completed, set state to idle
		renderThread->Set(RenderThread::State::Idle);

		performanceTimers.RenderThreadWorkTime = workTimer.ElapsedMillis();
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

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return s_Data->BlackTexture;
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

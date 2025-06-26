
#pragma once

#include "RendererContext.hpp"
#include "RenderCommandQueue.hpp"
#include "RenderCommandBuffer.hpp"

#include "RendererCapabilities.hpp"
#include "RendererConfig.hpp"
#include "RenderThread.hpp"

#include "Texture.hpp"

#include "GPUStats.hpp"

namespace Zenith {
	class Application;

	class Renderer
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		static Ref<RendererContext> GetContext()
		{
			return s_CurrentContext;
		}

		static void SetCurrentContext(Ref<RendererContext> context)
		{
			s_CurrentContext = context;
		}

		static void Init(Application* app);
		static void Shutdown();

		static RendererCapabilities& GetCapabilities();

		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
			};
			auto storageBuffer = GetRenderCommandQueue().Allocate(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
		}

		template<typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
			};

			if (RenderThread::IsCurrentThreadRT())
			{
				const uint32_t index = Renderer::RT_GetCurrentFrameIndex();
				auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
				new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
			}
			else
			{
				const uint32_t index = Renderer::GetCurrentFrameIndex();
				Submit([renderCmd, func, index]()
				{
					auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
					new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
				});
			}
		}

		static void WaitAndRender(RenderThread* renderThread);
		static void SwapQueues();

		static void RenderThreadFunc(RenderThread* renderThread);
		static uint32_t GetRenderQueueIndex();
		static uint32_t GetRenderQueueSubmissionIndex();

		// ~Actual~ Renderer here... TODO: remove confusion later

		// Render Pass API
		static void BeginFrame();
		static void EndFrame();

		static void BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor = {});
		static void InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor = {});
		static void EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer);

		static void RT_BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor = {});
		static void RT_InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor = {});
		static void RT_EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer);

		static void ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange = ImageSubresourceRange());
		static void CopyImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);
		static void BlitImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);

		static Ref<Texture2D> GetWhiteTexture();
		static Ref<Texture2D> GetBlackTexture();

		static uint32_t GetCurrentFrameIndex();
		static uint32_t RT_GetCurrentFrameIndex();

		static RendererConfig& GetConfig();
		static void SetConfig(const RendererConfig& config);

		static RenderCommandQueue& GetRenderResourceReleaseQueue(uint32_t index);

		static GPUMemoryStats GetGPUMemoryStats();
		static Application* GetApplication() { return s_Application; }
	private:
		static RenderCommandQueue& GetRenderCommandQueue();
		inline static Ref<RendererContext> s_CurrentContext = nullptr;
		static Application* s_Application;
	};

	namespace Utils {

		inline void DumpGPUInfo()
		{
			auto& caps = Renderer::GetCapabilities();
			ZN_CORE_TRACE_TAG("Renderer", "GPU Info:");
			ZN_CORE_TRACE_TAG("Renderer", "  Vendor: {0}", caps.Vendor);
			ZN_CORE_TRACE_TAG("Renderer", "  Device: {0}", caps.Device);
			ZN_CORE_TRACE_TAG("Renderer", "  Version: {0}", caps.Version);
		}

	}

}

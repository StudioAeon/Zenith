#pragma once

#include "RendererContext.hpp"
#include "RenderCommandQueue.hpp"

#include "Zenith/Core/Application.hpp"

#include "RendererCapabilities.hpp"

namespace Zenith {

	class Renderer
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		static Ref<RendererContext> GetContext()
		{
			return Application::Get().GetWindow().GetRenderContext();
		}

		static void Init();
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

		static void WaitAndRender();
		static void SwapQueues();

		static void BeginFrame();
		static void EndFrame();

		static uint32_t GetRenderQueueIndex();
		static uint32_t GetRenderQueueSubmissionIndex();

	private:
		static RenderCommandQueue& GetRenderCommandQueue();
	};

	namespace Utils {

		inline void DumpGPUInfo()
		{
			auto& caps = Renderer::GetCapabilities();
			ZN_CORE_TRACE("GPU Info:");
			ZN_CORE_TRACE("  Vendor: {0}", caps.Vendor);
			ZN_CORE_TRACE("  Device: {0}", caps.Device);
			ZN_CORE_TRACE("  Version: {0}", caps.Version);
		}

	}

}

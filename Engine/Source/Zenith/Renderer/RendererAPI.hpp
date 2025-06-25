#pragma once

#include "RendererCapabilities.hpp"
#include "RenderCommandBuffer.hpp"

namespace Zenith {

	enum class RendererAPIType
	{
		None,
		Vulkan
	};

	class RendererAPI
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& color) = 0;
		virtual void BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor) = 0;
		virtual void EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer) = 0;

		virtual void RT_InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& color) = 0;
		virtual void RT_BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor) = 0;
		virtual void RT_EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer) = 0;

		virtual void ClearImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange) = 0;
		virtual void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage) = 0;
		virtual void BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage) = 0;

		virtual RendererCapabilities& GetCapabilities() = 0;

		static RendererAPIType Current() { return s_CurrentRendererAPI; }
		static void SetAPI(RendererAPIType api);
	private:
		inline static RendererAPIType s_CurrentRendererAPI = RendererAPIType::Vulkan;
	};

}

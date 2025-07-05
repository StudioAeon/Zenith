#include "znpch.hpp"
#include "RenderCommandBuffer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanRenderCommandBuffer.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<RenderCommandBuffer> RenderCommandBuffer::Create(uint32_t count, const std::string& debugName)
	{
		return Ref<VulkanRenderCommandBuffer>::Create(count, debugName);
	}

	Ref<RenderCommandBuffer> RenderCommandBuffer::CreateFromSwapChain(const std::string& debugName)
	{
		return Ref<VulkanRenderCommandBuffer>::Create(debugName, true);
	}

}
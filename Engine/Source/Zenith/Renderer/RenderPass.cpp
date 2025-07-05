#include "znpch.hpp"
#include "RenderPass.hpp"

#include "Renderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanRenderPass.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		return Ref<VulkanRenderPass>::Create(spec);
	}

}
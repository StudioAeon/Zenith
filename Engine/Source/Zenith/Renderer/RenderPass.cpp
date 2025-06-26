#include "znpch.hpp"
#include "RenderPass.hpp"

#include "Renderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanRenderPass.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    ZN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderPass>::Create(spec);
		}

		ZN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
#include "znpch.hpp"
#include "RendererContext.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"

namespace Zenith {

	Ref<RendererContext> RendererContext::Create()
	{
		return Ref<VulkanContext>::Create();
	}

}
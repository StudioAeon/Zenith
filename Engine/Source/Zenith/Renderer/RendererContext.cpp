#include "znpch.hpp"
#include "RendererContext.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"

namespace Zenith {

	Ref<RendererContext> RendererContext::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create();
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
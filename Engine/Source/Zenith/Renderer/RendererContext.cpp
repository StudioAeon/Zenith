#include "znpch.hpp"
#include "RendererContext.hpp"

#include "RendererAPI.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"

namespace Zenith {

	Ref<RendererContext> RendererContext::Create(SDL_Window* windowHandle)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create(windowHandle);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}

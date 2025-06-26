#include "znpch.hpp"
#include "UniformBuffer.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanUniformBuffer.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:     return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanUniformBuffer>::Create(size);
		}

		ZN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}

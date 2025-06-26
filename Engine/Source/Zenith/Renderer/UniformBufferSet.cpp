#include "znpch.hpp"
#include "UniformBufferSet.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanUniformBufferSet.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<UniformBufferSet> UniformBufferSet::Create(uint32_t size, uint32_t framesInFlight)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:   return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanUniformBufferSet>::Create(size, framesInFlight);
		}

		ZN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
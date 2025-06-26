#include "znpch.hpp"

#include "UniformBufferSet.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "StorageBufferSet.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanStorageBufferSet.hpp"
#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<StorageBufferSet> StorageBufferSet::Create(const StorageBufferSpecification& specification, uint32_t size, uint32_t framesInFlight)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:   return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanStorageBufferSet>::Create(specification, size, framesInFlight);
		}

		ZN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
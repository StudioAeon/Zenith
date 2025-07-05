#include "znpch.hpp"

#include "UniformBufferSet.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "StorageBufferSet.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanStorageBufferSet.hpp"
#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<StorageBufferSet> StorageBufferSet::Create(const StorageBufferSpecification& specification, uint32_t size, uint32_t framesInFlight)
	{
		return Ref<VulkanStorageBufferSet>::Create(specification, size, framesInFlight);
	}

}
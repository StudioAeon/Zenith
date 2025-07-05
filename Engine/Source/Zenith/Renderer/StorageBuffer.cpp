#include "znpch.hpp"
#include "StorageBuffer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanStorageBuffer.hpp"
#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<StorageBuffer> StorageBuffer::Create(uint32_t size, const StorageBufferSpecification& specification)
	{
		return Ref<VulkanStorageBuffer>::Create(size, specification);
	}

}

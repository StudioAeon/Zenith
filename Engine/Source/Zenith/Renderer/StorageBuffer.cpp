#include "znpch.hpp"
#include "StorageBuffer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanStorageBuffer.hpp"
#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<StorageBuffer> StorageBuffer::Create(uint32_t size, const StorageBufferSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:     return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanStorageBuffer>::Create(size, specification);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}

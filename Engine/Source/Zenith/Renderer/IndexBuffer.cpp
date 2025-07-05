#include "znpch.hpp"
#include "IndexBuffer.hpp"

#include "Renderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanIndexBuffer.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<IndexBuffer> IndexBuffer::Create(uint64_t size)
	{
		return Ref<VulkanIndexBuffer>::Create(size);
	}

	Ref<IndexBuffer> IndexBuffer::Create(void* data, uint64_t size)
	{
		return Ref<VulkanIndexBuffer>::Create(data, size);
	}

}

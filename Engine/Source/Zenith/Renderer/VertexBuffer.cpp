#include "znpch.hpp"
#include "VertexBuffer.hpp"

#include "Renderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanVertexBuffer.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<VertexBuffer> VertexBuffer::Create(void* data, uint64_t size, VertexBufferUsage usage)
	{
		return Ref<VulkanVertexBuffer>::Create(data, size, usage);
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint64_t size, VertexBufferUsage usage)
	{
		return Ref<VulkanVertexBuffer>::Create(size, usage);
	}

}

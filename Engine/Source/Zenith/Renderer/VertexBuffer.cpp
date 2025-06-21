#include "znpch.hpp"
#include "VertexBuffer.hpp"

#include "Renderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanVertexBuffer.hpp"

namespace Zenith {

	Ref<VertexBuffer> VertexBuffer::Create(void* data, uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanVertexBuffer>::Create(data, size, usage);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanVertexBuffer>::Create(size, usage);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
#include "znpch.hpp"
#include "VulkanVertexBuffer.hpp"

namespace Zenith {

	VulkanVertexBuffer::VulkanVertexBuffer(const void* data, uint32_t size, VertexBufferUsage usage)
	: m_Usage(usage)
	{
		VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if (usage == VertexBufferUsage::Dynamic)
		{
			memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		m_Buffer = Ref<VulkanBuffer>::Create(bufferUsage, memoryProperties, size, data);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size, VertexBufferUsage usage)
		: VulkanVertexBuffer(nullptr, size, usage)
	{}

	void VulkanVertexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
	{
		m_Buffer->SetData(data, size, offset);
	}

	void VulkanVertexBuffer::Bind() const
	{
		// Vulkan binding is done through command buffer, not state machine
		// This will be handled in the render commands
	}

	Ref<VulkanVertexBuffer> VulkanVertexBuffer::Create(const void* data, uint32_t size, VertexBufferUsage usage)
	{
		return Ref<VulkanVertexBuffer>::Create(data, size, usage);
	}

	Ref<VulkanVertexBuffer> VulkanVertexBuffer::Create(uint32_t size, VertexBufferUsage usage)
	{
		return Ref<VulkanVertexBuffer>::Create(size, usage);
	}

}
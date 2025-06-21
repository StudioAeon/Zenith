#include "znpch.hpp"
#include "VulkanIndexBuffer.hpp"

namespace Zenith {

	VulkanIndexBuffer::VulkanIndexBuffer(const void* data, uint32_t size)
	{
		m_Buffer = Ref<VulkanBuffer>::Create(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			size,
			data
		);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t size)
		: VulkanIndexBuffer(nullptr, size)
	{
	}

	void VulkanIndexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
	{
		m_Buffer->SetData(data, size, offset);
	}

	void VulkanIndexBuffer::Bind() const
	{
		// Vulkan binding is done through command buffer
	}

	Ref<VulkanIndexBuffer> VulkanIndexBuffer::Create(const void* data, uint32_t size)
	{
		return Ref<VulkanIndexBuffer>::Create(data, size);
	}

	Ref<VulkanIndexBuffer> VulkanIndexBuffer::Create(uint32_t size)
	{
		return Ref<VulkanIndexBuffer>::Create(size);
	}

}
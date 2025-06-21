#include "znpch.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanContext.hpp"

namespace Zenith {

	VulkanBuffer::VulkanBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, uint32_t size, const void* data)
		: m_UsageFlags(usage), m_MemoryPropertyFlags(memoryProperties), m_Size(size)
	{
		m_Device = VulkanContext::GetCurrentDevice();

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK_RESULT(vkCreateBuffer(m_Device->GetVulkanDevice(), &bufferCreateInfo, nullptr, &m_Buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_Device->GetVulkanDevice(), m_Buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memoryProperties);

		VK_CHECK_RESULT(vkAllocateMemory(m_Device->GetVulkanDevice(), &allocInfo, nullptr, &m_Memory));

		VK_CHECK_RESULT(vkBindBufferMemory(m_Device->GetVulkanDevice(), m_Buffer, m_Memory, 0));

		if (data)
		{
			SetData(data, size, 0);
		}
	}

	VulkanBuffer::~VulkanBuffer()
	{
		Release();
	}

	void VulkanBuffer::Allocate(uint32_t size)
	{
		Release();
		m_Size = size;

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = m_UsageFlags;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK_RESULT(vkCreateBuffer(m_Device->GetVulkanDevice(), &bufferCreateInfo, nullptr, &m_Buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_Device->GetVulkanDevice(), m_Buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, m_MemoryPropertyFlags);

		VK_CHECK_RESULT(vkAllocateMemory(m_Device->GetVulkanDevice(), &allocInfo, nullptr, &m_Memory));

		VK_CHECK_RESULT(vkBindBufferMemory(m_Device->GetVulkanDevice(), m_Buffer, m_Memory, 0));
	}

	void VulkanBuffer::Release()
	{
		if (m_Mapped)
		{
			Unmap();
		}

		if (m_Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(m_Device->GetVulkanDevice(), m_Buffer, nullptr);
			m_Buffer = VK_NULL_HANDLE;
		}

		if (m_Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_Device->GetVulkanDevice(), m_Memory, nullptr);
			m_Memory = VK_NULL_HANDLE;
		}
	}

	void VulkanBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		ZN_CORE_ASSERT(data, "Data is null!");
		ZN_CORE_ASSERT(offset + size <= m_Size, "Buffer overflow!");

		if (m_MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT &&
			!(m_MemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		{
			VulkanBuffer stagingBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				size,
				data
			);

			stagingBuffer.CopyTo(this, 0, offset, size);
		}
		else
		{
			void* mapped = Map();
			memcpy(static_cast<uint8_t*>(mapped) + offset, data, size);
			Unmap();
		}
	}

	void* VulkanBuffer::Map()
	{
		ZN_CORE_ASSERT(!m_Mapped, "Buffer is already mapped!");

		void* data;
		VK_CHECK_RESULT(vkMapMemory(m_Device->GetVulkanDevice(), m_Memory, 0, m_Size, 0, &data));
		m_Mapped = true;
		return data;
	}

	void VulkanBuffer::Unmap()
	{
		ZN_CORE_ASSERT(m_Mapped, "Buffer is not mapped!");

		vkUnmapMemory(m_Device->GetVulkanDevice(), m_Memory);
		m_Mapped = false;
	}

	void VulkanBuffer::CopyTo(VulkanBuffer* dst, uint32_t srcOffset, uint32_t dstOffset, uint32_t size)
	{
		if (size == 0)
			size = m_Size - srcOffset;

		ZN_CORE_ASSERT(srcOffset + size <= m_Size, "Source buffer overflow!");
		ZN_CORE_ASSERT(dstOffset + size <= dst->GetSize(), "Destination buffer overflow!");

		VkCommandBuffer commandBuffer = m_Device->GetCommandBuffer(true);

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, m_Buffer, dst->GetVulkanBuffer(), 1, &copyRegion);

		m_Device->FlushCommandBuffer(commandBuffer);
	}

	uint32_t VulkanBuffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_Device->GetPhysicalDevice()->GetVulkanPhysicalDevice(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		ZN_CORE_ASSERT(false, "Failed to find suitable memory type!");
		return 0;
	}

}
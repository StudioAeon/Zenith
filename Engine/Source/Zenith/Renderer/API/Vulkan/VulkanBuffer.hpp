#pragma once

#include "Zenith/Core/Ref.hpp"
#include "Vulkan.hpp"
#include "VulkanDevice.hpp"

namespace Zenith {

	class VulkanBuffer : public RefCounted
	{
	public:
		VulkanBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, uint32_t size, const void* data = nullptr);
		~VulkanBuffer();

		void Allocate(uint32_t size);
		void Release();

		void SetData(const void* data, uint32_t size, uint32_t offset = 0);
		void* Map();
		void Unmap();

		void CopyTo(VulkanBuffer* dst, uint32_t srcOffset = 0, uint32_t dstOffset = 0, uint32_t size = 0);

		VkBuffer GetVulkanBuffer() const { return m_Buffer; }
		VkDeviceMemory GetVulkanDeviceMemory() const { return m_Memory; }
		uint32_t GetSize() const { return m_Size; }

	private:
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VkBufferUsageFlags m_UsageFlags;
		VkMemoryPropertyFlags m_MemoryPropertyFlags;
		uint32_t m_Size = 0;
		bool m_Mapped = false;
		Ref<VulkanDevice> m_Device;
	};

}
#include "znpch.hpp"
#include "VulkanAllocator.hpp"

#include "VulkanContext.hpp"

namespace Zenith {

	VulkanAllocator::VulkanAllocator(const Ref<VulkanDevice>& device, const std::string& tag)
		: m_Device(device), m_Tag(tag)
	{
	}

	VulkanAllocator::VulkanAllocator(const std::string& tag)
		: m_Device(VulkanContext::GetCurrentDevice()), m_Tag(tag)
	{
	}

	VulkanAllocator::~VulkanAllocator()
	{
	}

	void VulkanAllocator::Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/)
	{
		ZN_CORE_ASSERT(m_Device);

		// TODO: Tracking
		ZN_CORE_TRACE("VulkanAllocator ({0}): allocating {1} bytes", m_Tag, requirements.size);

		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = requirements.size;
		memAlloc.memoryTypeIndex = m_Device->GetPhysicalDevice()->GetMemoryTypeIndex(requirements.memoryTypeBits, flags);
		VK_CHECK_RESULT(vkAllocateMemory(m_Device->GetVulkanDevice(), &memAlloc, nullptr, dest));
	}

}
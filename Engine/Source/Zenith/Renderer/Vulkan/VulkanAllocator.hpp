#pragma once

#include <string>
#include "Vulkan.hpp"
#include "VulkanDevice.hpp"

namespace Zenith {

	class VulkanAllocator
	{
	public:
		VulkanAllocator() = default;
		VulkanAllocator(const std::string& tag);
		VulkanAllocator(const Ref<VulkanDevice>& device, const std::string& tag = "");
		~VulkanAllocator();

		void Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	private:
		Ref<VulkanDevice> m_Device;
		std::string m_Tag;
	};


}
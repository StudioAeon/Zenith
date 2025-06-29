#include "znpch.hpp"
#include "VulkanDevice.hpp"

#include "VulkanContext.hpp"
#include "Zenith/Core/Assert.hpp"

namespace Zenith {

	////////////////////////////////////////////////////////////////////////////////////
	// Vulkan Physical Device
	////////////////////////////////////////////////////////////////////////////////////

	VulkanPhysicalDevice::VulkanPhysicalDevice()
	{
		auto vkInstance = VulkanContext::GetInstance();

		uint32_t gpuCount = 0;
		// Get number of available physical devices
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &gpuCount, nullptr));
		ZN_CORE_ASSERT(gpuCount > 0, "");
		// Enumerate physical devices
		std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &gpuCount, physicalDevices.data()));

		VkPhysicalDevice selectedPhysicalDevice = nullptr;
		for (VkPhysicalDevice device : physicalDevices)
		{
			vkGetPhysicalDeviceProperties(device, &m_Properties);
			if (m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				selectedPhysicalDevice = device;
				break;
			}
		}

		if (!selectedPhysicalDevice)
		{
			std::cout << "[Renderer] Warning: Could not find discrete GPU, using first available device\n";
			selectedPhysicalDevice = physicalDevices.back();
		}

		ZN_CORE_ASSERT(selectedPhysicalDevice, "Could not find any physical devices!");
		m_PhysicalDevice = selectedPhysicalDevice;

		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_Features);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
		ZN_CORE_ASSERT(queueFamilyCount > 0, "");
		m_QueueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.data());

		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				ZN_CORE_INFO_TAG("Renderer", "Selected physical device has {0} extensions", extensions.size());
				for (const auto& ext : extensions)
				{
					m_SupportedExtensions.emplace(ext.extensionName);
					ZN_CORE_INFO_TAG("Renderer", "  {0}", ext.extensionName);
				}
			}
		}

		// Queue families
		// Desired queues need to be requested upon logical device creation
		// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
		// requests different queue types

		// Get queue family indices for the requested queue family types
		// Note that the indices may overlap depending on the implementation

		static const float defaultQueuePriority(0.0f);

		int requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		m_QueueFamilyIndices = GetQueueFamilyIndices(requestedQueueTypes);

		// Graphics queue
		if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
		{
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Graphics;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			m_QueueCreateInfos.push_back(queueInfo);
		}

		// Dedicated compute queue
		if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
		{
			if (m_QueueFamilyIndices.Compute != m_QueueFamilyIndices.Graphics)
			{
				// If compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Compute;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				m_QueueCreateInfos.push_back(queueInfo);
			}
		}

		// Dedicated transfer queue
		if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
		{
			if ((m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Graphics) && (m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Compute))
			{
				// If compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Transfer;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				m_QueueCreateInfos.push_back(queueInfo);
			}
		}

		m_DepthFormat = FindDepthFormat();
		ZN_CORE_ASSERT(m_DepthFormat);
	}

	VulkanPhysicalDevice::~VulkanPhysicalDevice()
	{
	}

	VkFormat VulkanPhysicalDevice::FindDepthFormat() const
	{
		// Since all depth formats may be optional, we need to find a suitable depth format to use
		// Start with the highest precision packed format
		std::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		// TODO: Move to VulkanPhysicalDevice
		for (auto& format : depthFormats)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &formatProps);
			// Format must support depth stencil attachment for optimal tiling
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				return format;
		}
		return VK_FORMAT_UNDEFINED;
	}

	bool VulkanPhysicalDevice::IsExtensionSupported(const std::string& extensionName) const
	{
		return m_SupportedExtensions.find(extensionName) != m_SupportedExtensions.end();
	}

	VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::GetQueueFamilyIndices(int flags)
	{
		QueueFamilyIndices indices;

		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
		if (flags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = m_QueueFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					indices.Compute = i;
					break;
				}
			}
		}

		// Dedicated queue for transfer
		// Try to find a queue family index that supports transfer but not graphics and compute
		if (flags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = m_QueueFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					indices.Transfer = i;
					break;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
		{
			if ((flags & VK_QUEUE_TRANSFER_BIT) && indices.Transfer == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
					indices.Transfer = i;
			}

			if ((flags & VK_QUEUE_COMPUTE_BIT) && indices.Compute == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
					indices.Compute = i;
			}

			if (flags & VK_QUEUE_GRAPHICS_BIT)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.Graphics = i;
			}
		}

		return indices;
	}

	uint32_t VulkanPhysicalDevice::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const
	{
		// Iterate over all memory types available for the device used in this example
		for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
					return i;
			}
			typeBits >>= 1;
		}

		ZN_CORE_ASSERT(false, "Could not find a suitable memory type!");
		return UINT32_MAX;
	}

	bool VulkanPhysicalDevice::IsIntelGPU() const
	{
		return m_Properties.vendorID == 0x8086; // Intel vendor ID
	}

	void VulkanPhysicalDevice::LogDeviceInfo() const
	{
		ZN_CORE_INFO_TAG("Renderer", "=== Vulkan Device Info ===");
		ZN_CORE_INFO_TAG("Renderer", "Device: {}", m_Properties.deviceName);
		ZN_CORE_INFO_TAG("Renderer", "Vendor: 0x{:X}", m_Properties.vendorID);
		ZN_CORE_INFO_TAG("Renderer", "Driver: {}", m_Properties.driverVersion);
		ZN_CORE_INFO_TAG("Renderer", "API Version: {}.{}.{}",
			VK_VERSION_MAJOR(m_Properties.apiVersion),
			VK_VERSION_MINOR(m_Properties.apiVersion),
			VK_VERSION_PATCH(m_Properties.apiVersion));

		// Log critical features
		ZN_CORE_INFO_TAG("Renderer", "Anisotropy: {}", m_Features.samplerAnisotropy ? "Yes" : "No");
		ZN_CORE_INFO_TAG("Renderer", "Wide Lines: {}", m_Features.wideLines ? "Yes" : "No");
		ZN_CORE_INFO_TAG("Renderer", "Fill Mode Non-Solid: {}", m_Features.fillModeNonSolid ? "Yes" : "No");
		ZN_CORE_INFO_TAG("Renderer", "Independent Blend: {}", m_Features.independentBlend ? "Yes" : "No");
		ZN_CORE_INFO_TAG("Renderer", "Pipeline Statistics: {}", m_Features.pipelineStatisticsQuery ? "Yes" : "No");
		ZN_CORE_INFO_TAG("Renderer", "Shader Storage Image Read Without Format: {}", m_Features.shaderStorageImageReadWithoutFormat ? "Yes" : "No");
	}

	Ref<VulkanPhysicalDevice> VulkanPhysicalDevice::Select()
	{
		return Ref<VulkanPhysicalDevice>::Create();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Vulkan Device
	////////////////////////////////////////////////////////////////////////////////////

	VulkanDevice::VulkanDevice(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures)
		: m_PhysicalDevice(physicalDevice), m_EnabledFeatures(enabledFeatures)
	{
		// Log device information for debugging
		m_PhysicalDevice->LogDeviceInfo();

		// Do we need to enable any other extensions (eg. NV_RAYTRACING?)
		std::vector<const char*> deviceExtensions;

		// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
		ZN_CORE_ASSERT(m_PhysicalDevice->IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME));
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		// Add optional extensions only if supported
		if (m_PhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME))
			deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
		if (m_PhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME))
			deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);

		// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
		if (m_PhysicalDevice->IsExtensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
		{
			deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
			m_EnableDebugMarkers = true;
		}

		// Validate and adjust features for device compatibility
		VkPhysicalDeviceFeatures safeFeatures = ValidateFeaturesForDevice(enabledFeatures);

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(physicalDevice->m_QueueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = physicalDevice->m_QueueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &safeFeatures;

		if (deviceExtensions.size() > 0)
		{
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		// Attempt device creation with full feature set
		VkResult result = vkCreateDevice(m_PhysicalDevice->GetVulkanPhysicalDevice(), &deviceCreateInfo, nullptr, &m_LogicalDevice);

		if (result != VK_SUCCESS) {
			ZN_CORE_ERROR_TAG("Renderer", "Failed to create Vulkan device: {}", Utils::VKResultToString(result));
			ZN_CORE_ERROR_TAG("Renderer", "Device: {} (Driver: {})",
				m_PhysicalDevice->GetProperties().deviceName,
				m_PhysicalDevice->GetProperties().driverVersion);

			// Fallback: try with minimal features
			if (!TryCreateDeviceWithMinimalFeatures(deviceExtensions)) {
				ZN_CORE_ERROR_TAG("Renderer", "Failed to create Vulkan device even with minimal features");
				ZN_CORE_ASSERT(false, "Vulkan device creation failed completely");
				return;
			}
		} else {
			// Store the successfully enabled features
			m_EnabledFeatures = safeFeatures;
			ZN_CORE_INFO_TAG("Renderer", "Vulkan device created successfully with requested features");
		}

		// Get queues from the device
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Compute, 0, &m_ComputeQueue);
	}

	VulkanDevice::~VulkanDevice()
	{
	}

	void VulkanDevice::Destroy()
	{
		m_CommandPools.clear();
		vkDeviceWaitIdle(m_LogicalDevice);
		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	void VulkanDevice::LockQueue(bool compute)
	{
		if (compute)
			m_ComputeQueueMutex.lock();
		else
			m_GraphicsQueueMutex.lock();
	}

	void VulkanDevice::UnlockQueue(bool compute)
	{
		if (compute)
			m_ComputeQueueMutex.unlock();
		else
			m_GraphicsQueueMutex.unlock();
	}

	VkCommandBuffer VulkanDevice::GetCommandBuffer(bool begin, bool compute)
	{
		return GetOrCreateThreadLocalCommandPool()->AllocateCommandBuffer(begin, compute);
	}

	void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer)
	{
		GetThreadLocalCommandPool()->FlushCommandBuffer(commandBuffer);
	}

	void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
	{
		GetThreadLocalCommandPool()->FlushCommandBuffer(commandBuffer);
	}

	VkCommandBuffer VulkanDevice::CreateSecondaryCommandBuffer(const char* debugName)
	{
		VkCommandBuffer cmdBuffer;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = GetOrCreateThreadLocalCommandPool()->GetGraphicsCommandPool();
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		cmdBufAllocateInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
		VKUtils::SetDebugUtilsObjectName(m_LogicalDevice, VK_OBJECT_TYPE_COMMAND_BUFFER, debugName, cmdBuffer);
		return cmdBuffer;
	}

	VkPhysicalDeviceFeatures VulkanDevice::ValidateFeaturesForDevice(const VkPhysicalDeviceFeatures& requestedFeatures)
	{
		const VkPhysicalDeviceFeatures& deviceFeatures = m_PhysicalDevice->GetFeatures();
		VkPhysicalDeviceFeatures safeFeatures = {};

		// Only enable features that are actually supported by the device
		safeFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy && requestedFeatures.samplerAnisotropy;
		safeFeatures.wideLines = deviceFeatures.wideLines && requestedFeatures.wideLines;
		safeFeatures.fillModeNonSolid = deviceFeatures.fillModeNonSolid && requestedFeatures.fillModeNonSolid;
		safeFeatures.independentBlend = deviceFeatures.independentBlend && requestedFeatures.independentBlend;
		safeFeatures.pipelineStatisticsQuery = deviceFeatures.pipelineStatisticsQuery && requestedFeatures.pipelineStatisticsQuery;
		safeFeatures.shaderStorageImageReadWithoutFormat = deviceFeatures.shaderStorageImageReadWithoutFormat && requestedFeatures.shaderStorageImageReadWithoutFormat;

		// Special handling for Intel integrated graphics
		if (m_PhysicalDevice->IsIntelGPU()) {
			ZN_CORE_WARN_TAG("Renderer", "Intel integrated graphics detected - applying conservative feature selection");

			// Intel iGPUs often have issues with these features
			if (!deviceFeatures.wideLines) {
				safeFeatures.wideLines = false;
				ZN_CORE_WARN_TAG("Renderer", "Disabled wide lines for Intel GPU");
			}

			if (!deviceFeatures.fillModeNonSolid) {
				safeFeatures.fillModeNonSolid = false;
				ZN_CORE_WARN_TAG("Renderer", "Disabled fill mode non-solid for Intel GPU");
			}
		}

		// Log which features were disabled
		if (requestedFeatures.samplerAnisotropy && !safeFeatures.samplerAnisotropy)
			ZN_CORE_WARN_TAG("Renderer", "Sampler anisotropy not supported by device");
		if (requestedFeatures.wideLines && !safeFeatures.wideLines)
			ZN_CORE_WARN_TAG("Renderer", "Wide lines not supported by device");
		if (requestedFeatures.fillModeNonSolid && !safeFeatures.fillModeNonSolid)
			ZN_CORE_WARN_TAG("Renderer", "Fill mode non-solid not supported by device");
		if (requestedFeatures.independentBlend && !safeFeatures.independentBlend)
			ZN_CORE_WARN_TAG("Renderer", "Independent blend not supported by device");
		if (requestedFeatures.pipelineStatisticsQuery && !safeFeatures.pipelineStatisticsQuery)
			ZN_CORE_WARN_TAG("Renderer", "Pipeline statistics query not supported by device");
		if (requestedFeatures.shaderStorageImageReadWithoutFormat && !safeFeatures.shaderStorageImageReadWithoutFormat)
			ZN_CORE_WARN_TAG("Renderer", "Shader storage image read without format not supported by device");

		return safeFeatures;
	}

	bool VulkanDevice::TryCreateDeviceWithMinimalFeatures(const std::vector<const char*>& deviceExtensions)
	{
		ZN_CORE_WARN_TAG("Renderer", "Attempting device creation with minimal features...");

		// Minimal feature set that should work on most devices
		VkPhysicalDeviceFeatures minimalFeatures = {};

		// Only enable anisotropy if supported (most basic feature)
		const VkPhysicalDeviceFeatures& deviceFeatures = m_PhysicalDevice->GetFeatures();
		minimalFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;

		// Recreate device info with minimal features
		VkDeviceCreateInfo minimalCreateInfo = {};
		minimalCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		minimalCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(m_PhysicalDevice->m_QueueCreateInfos.size());
		minimalCreateInfo.pQueueCreateInfos = m_PhysicalDevice->m_QueueCreateInfos.data();
		minimalCreateInfo.pEnabledFeatures = &minimalFeatures;

		// Only use essential extensions for fallback
		std::vector<const char*> essentialExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		minimalCreateInfo.enabledExtensionCount = static_cast<uint32_t>(essentialExtensions.size());
		minimalCreateInfo.ppEnabledExtensionNames = essentialExtensions.data();

		VkResult result = vkCreateDevice(m_PhysicalDevice->GetVulkanPhysicalDevice(), &minimalCreateInfo, nullptr, &m_LogicalDevice);
		if (result == VK_SUCCESS) {
			ZN_CORE_WARN_TAG("Renderer", "Device created with minimal features");
			m_EnabledFeatures = minimalFeatures;
			m_EnableDebugMarkers = false; // Disabled in minimal mode
			return true;
		}

		ZN_CORE_ERROR_TAG("Renderer", "Minimal device creation also failed: {}", Utils::VKResultToString(result));
		return false;
	}

	Ref<VulkanCommandPool> VulkanDevice::GetThreadLocalCommandPool()
	{
		auto threadID = std::this_thread::get_id();
		ZN_CORE_VERIFY(m_CommandPools.find(threadID) != m_CommandPools.end());

		return m_CommandPools.at(threadID);
	}

	Ref<VulkanCommandPool> VulkanDevice::GetOrCreateThreadLocalCommandPool()
	{
		auto threadID = std::this_thread::get_id();
		auto commandPoolIt = m_CommandPools.find(threadID);
		if (commandPoolIt != m_CommandPools.end())
			return commandPoolIt->second;

		Ref<VulkanCommandPool> commandPool = Ref<VulkanCommandPool>::Create();
		m_CommandPools[threadID] = commandPool;
		return commandPool;
	}

	VulkanCommandPool::VulkanCommandPool()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(vulkanDevice, &cmdPoolInfo, nullptr, &m_GraphicsCommandPool));

		cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Compute;
		VK_CHECK_RESULT(vkCreateCommandPool(vulkanDevice, &cmdPoolInfo, nullptr, &m_ComputeCommandPool));
	}

	VulkanCommandPool::~VulkanCommandPool()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		vkDestroyCommandPool(vulkanDevice, m_GraphicsCommandPool, nullptr);
		vkDestroyCommandPool(vulkanDevice, m_ComputeCommandPool, nullptr);
	}

	VkCommandBuffer VulkanCommandPool::AllocateCommandBuffer(bool begin, bool compute)
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkCommandBuffer cmdBuffer;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = compute ? m_ComputeCommandPool : m_GraphicsCommandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice, &cmdBufAllocateInfo, &cmdBuffer));

		// If requested, also start the new command buffer
		if (begin)
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo{};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
		}

		return cmdBuffer;
	}

	void VulkanCommandPool::FlushCommandBuffer(VkCommandBuffer commandBuffer)
	{
		auto device = VulkanContext::GetCurrentDevice();
		FlushCommandBuffer(commandBuffer, device->GetGraphicsQueue());
	}

	void VulkanCommandPool::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
	{
		auto device = VulkanContext::GetCurrentDevice();
		ZN_CORE_VERIFY(queue == device->GetGraphicsQueue());
		auto vulkanDevice = device->GetVulkanDevice();

		const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;

		ZN_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE);

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = 0;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(vulkanDevice, &fenceCreateInfo, nullptr, &fence));

		{
			device->LockQueue();

			// Submit to the queue
			VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
			
			device->UnlockQueue();
		}
		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK_RESULT(vkWaitForFences(vulkanDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		vkDestroyFence(vulkanDevice, fence, nullptr);
		vkFreeCommandBuffers(vulkanDevice, m_GraphicsCommandPool, 1, &commandBuffer);
	}

}
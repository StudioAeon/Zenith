#include "znpch.hpp"
#include "VulkanDiagnostics.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"

namespace Zenith::Utils {

	static std::vector<VulkanCheckpointData> s_CheckpointStorage(1024);
	static uint32_t s_CheckpointStorageIndex = 0;

	void SetVulkanCheckpoint(VkCommandBuffer commandBuffer, const std::string& data)
	{
		const bool supported = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
		if (!supported)
			return;

		s_CheckpointStorageIndex = (s_CheckpointStorageIndex + 1) % 1024;
		VulkanCheckpointData& checkpoint = s_CheckpointStorage[s_CheckpointStorageIndex];

		const size_t copyLen = std::min(data.size(), sizeof(checkpoint.Data) - 1);
		std::memcpy(checkpoint.Data, data.data(), copyLen);
		checkpoint.Data[copyLen] = '\0';

		vkCmdSetCheckpointNV(commandBuffer, &checkpoint);
	}

}
#pragma once

#include <filesystem>
#include <map>

#include "VulkanShaderCompiler.hpp"

namespace Zenith {

	class VulkanShaderCache
	{
	public:
		static VkShaderStageFlagBits HasChanged(Ref<VulkanShaderCompiler> shader);
	private:
		static void Serialize(const std::map<std::string, std::map<VkShaderStageFlagBits, StageData>>& shaderCache);
		static void Deserialize(std::map<std::string, std::map<VkShaderStageFlagBits, StageData>>& shaderCache);
	};

}

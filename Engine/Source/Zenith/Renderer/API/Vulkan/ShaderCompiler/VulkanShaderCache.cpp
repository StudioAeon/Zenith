#include "znpch.hpp"
#include "VulkanShaderCache.hpp"
#include "Zenith/Core/Hash.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanShaderUtils.hpp"

#include <nlohmann/json.hpp>
#include "Zenith/Utilities/SerializationMacros.hpp"

#include "ShaderPreprocessing/ShaderPreprocessor.hpp"

namespace Zenith {

	static const char* s_ShaderRegistryPath = "Resources/Cache/Shader/ShaderRegistry.cache";

	VkShaderStageFlagBits VulkanShaderCache::HasChanged(Ref<VulkanShaderCompiler> shader)
	{
		std::map<std::string, std::map<VkShaderStageFlagBits, StageData>> shaderCache;

		Deserialize(shaderCache);

		VkShaderStageFlagBits changedStages = {};
		const bool shaderNotCached = shaderCache.find(shader->m_ShaderSourcePath.string()) == shaderCache.end();

		for (const auto& [stage, stageSource] : shader->m_ShaderSource)
		{
			// Keep in mind that we're using the [] operator.
			// Which means that we add the stage if it's not already there.
			if (shaderNotCached || shader->m_StagesMetadata.at(stage) != shaderCache[shader->m_ShaderSourcePath.string()][stage])
			{
				shaderCache[shader->m_ShaderSourcePath.string()][stage] = shader->m_StagesMetadata.at(stage);
				*(int*)&changedStages |= stage;
			}
		}

		// Update cache in case we added a stage but didn't remove the deleted(in file) stages
		shaderCache.at(shader->m_ShaderSourcePath.string()) = shader->m_StagesMetadata;

		if (changedStages)
		{
			Serialize(shaderCache);
		}

		return changedStages;
	}

	void VulkanShaderCache::Serialize(const std::map<std::string, std::map<VkShaderStageFlagBits, StageData>>& shaderCache)
	{
		nlohmann::json json;
		auto& shaderRegistry = json["ShaderRegistry"] = nlohmann::json::array();

		for (const auto& [filepath, shader] : shaderCache)
		{
			nlohmann::json shaderData;
			shaderData["ShaderPath"] = filepath;

			auto& stages = shaderData["Stages"] = nlohmann::json::array();

			for (const auto& [stage, stageData] : shader)
			{
				nlohmann::json stageObj;
				stageObj["Stage"] = ShaderUtils::ShaderStageToString(stage);
				stageObj["StageHash"] = stageData.HashValue;

				auto& headers = stageObj["Headers"] = nlohmann::json::array();
				for (const auto& header : stageData.Headers)
				{
					nlohmann::json headerObj;
					headerObj["HeaderPath"] = header.IncludedFilePath.string();
					headerObj["IncludeDepth"] = header.IncludeDepth;
					headerObj["IsRelative"] = header.IsRelative;
					headerObj["IsGaurded"] = header.IsGuarded;
					headerObj["HashValue"] = header.HashValue;

					headers.push_back(headerObj);
				}

				stages.push_back(stageObj);
			}

			shaderRegistry.push_back(shaderData);
		}

		std::ofstream fout(s_ShaderRegistryPath);
		fout << json.dump(4); // Pretty print with 4 space indentation
	}

	void VulkanShaderCache::Deserialize(std::map<std::string, std::map<VkShaderStageFlagBits, StageData>>& shaderCache)
	{
		// Read registry
		std::ifstream stream(s_ShaderRegistryPath);
		if (!stream.good())
			return;

		try
		{
			nlohmann::json json;
			stream >> json;

			if (!json.contains("ShaderRegistry"))
			{
				ZN_CORE_ERROR("[ShaderCache] Shader Registry is invalid - missing 'ShaderRegistry' key.");
				return;
			}

			const auto& shaderRegistry = json["ShaderRegistry"];
			if (!shaderRegistry.is_array())
			{
				ZN_CORE_ERROR("[ShaderCache] Invalid Shader Registry format - 'ShaderRegistry' should be an array.");
				return;
			}

			for (const auto& shader : shaderRegistry)
			{
				if (!shader.contains("ShaderPath") || !shader.contains("Stages"))
				{
					ZN_CORE_WARN("[ShaderCache] Skipping invalid shader entry - missing required fields.");
					continue;
				}

				const std::string path = shader["ShaderPath"];
				const auto& stages = shader["Stages"];

				if (!stages.is_array())
				{
					ZN_CORE_WARN("[ShaderCache] Skipping shader '{}' - invalid stages format.", path);
					continue;
				}

				for (const auto& stage : stages)
				{
					if (!stage.contains("Stage") || !stage.contains("StageHash"))
					{
						ZN_CORE_WARN("[ShaderCache] Skipping invalid stage in shader '{}'.", path);
						continue;
					}

					const std::string stageType = stage["Stage"];
					const uint32_t stageHash = stage["StageHash"];

					auto& stageCache = shaderCache[path][ShaderUtils::ShaderTypeFromString(stageType)];
					stageCache.HashValue = stageHash;

					if (stage.contains("Headers") && stage["Headers"].is_array())
					{
						for (const auto& header : stage["Headers"])
						{
							if (!header.contains("HeaderPath"))
								continue;

							const std::string headerPath = header["HeaderPath"];
							const uint32_t includeDepth = header.value("IncludeDepth", 0u);
							const bool isRelative = header.value("IsRelative", false);
							const bool isGuarded = header.value("IsGaurded", false);
							const uint32_t hashValue = header.value("HashValue", 0u);

							stageCache.Headers.emplace(IncludeData{ headerPath, includeDepth, isRelative, isGuarded, hashValue });
						}
					}
				}
			}
		}
		catch (const nlohmann::json::exception& e)
		{
			ZN_CORE_ERROR("[ShaderCache] Failed to parse JSON: {}", e.what());
		}
	}

}
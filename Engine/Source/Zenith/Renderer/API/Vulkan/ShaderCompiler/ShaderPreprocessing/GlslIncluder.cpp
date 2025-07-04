#include "znpch.hpp"
#include "GlslIncluder.hpp"

#include "Zenith/Utilities/StringUtils.hpp"
#include <filesystem>
#include "Zenith/Core/Hash.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanShader.hpp"

namespace Zenith {

	GlslIncluder::GlslIncluder(Utils::IncludePathManager* pathManager)
		: m_PathManager(pathManager)
	{
	}

	GlslIncluder::~GlslIncluder() = default;

	shaderc_include_result* GlslIncluder::GetInclude(const char* requestedPath, const shaderc_include_type type, const char* requestingPath, const size_t includeDepth)
	{
		std::filesystem::path requestedFullPath;

		if (type == shaderc_include_type_relative)
		{
			requestedFullPath = m_PathManager->FindRelativeIncludeFile(requestingPath, requestedPath);
		}
		else // shaderc_include_type_standard
		{
			requestedFullPath = m_PathManager->FindIncludeFile(requestedPath);
		}

		// Check if file was found
		if (requestedFullPath.empty())
		{
			ZN_CORE_ERROR("Failed to find included file: {} requested from {}", requestedPath, requestingPath);

			// Return empty result for failed includes
			auto* const container = new std::array<std::string, 2>;
			(*container)[0] = requestedPath;
			(*container)[1] = ""; // Empty content

			auto* const data = new shaderc_include_result;
			data->user_data = container;
			data->source_name = (*container)[0].data();
			data->source_name_length = (*container)[0].size();
			data->content = (*container)[1].data();
			data->content_length = (*container)[1].size();

			return data;
		}

		auto& [source, sourceHash, stages, isGuarded] = m_HeaderCache[requestedFullPath.string()];

		if (source.empty())
		{
			source = Utils::ReadFileAndSkipBOM(requestedFullPath);
			if (source.empty())
			{
				ZN_CORE_ERROR("Failed to load included file: {} in {}.", requestedFullPath.string(), requestingPath);
			}
			sourceHash = Hash::GenerateFNVHash(source.c_str());

			// Can clear "source" in case it has already been included in this stage and is guarded.
			stages = ShaderPreprocessor::PreprocessHeader<ShaderUtils::SourceLang::GLSL>(source, isGuarded, m_ParsedSpecialMacros, m_includeData, requestedFullPath);
		}
		else if (isGuarded)
		{
			source.clear();
		}

		// Does not emplace if it finds the same include path and same header hash value.
		m_includeData.emplace(IncludeData{
			requestedFullPath,
			includeDepth,
			type == shaderc_include_type_relative,
			isGuarded,
			sourceHash,
			stages
		});

		auto* const container = new std::array<std::string, 2>;
		(*container)[0] = requestedPath;
		(*container)[1] = source;
		auto* const data = new shaderc_include_result;

		data->user_data = container;

		data->source_name = (*container)[0].data();
		data->source_name_length = (*container)[0].size();

		data->content = (*container)[1].data();
		data->content_length = (*container)[1].size();

		return data;
	}

	void GlslIncluder::ReleaseInclude(shaderc_include_result* data)
	{
		delete static_cast<std::array<std::string, 2>*>(data->user_data);
		delete data;
	}

}
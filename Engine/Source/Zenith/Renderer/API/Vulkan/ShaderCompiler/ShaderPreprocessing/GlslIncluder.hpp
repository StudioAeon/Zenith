#pragma once

#include <unordered_set>
#include <shaderc/shaderc.hpp>
#include <filesystem>

#include "IncludePathManager.hpp"
#include "ShaderPreprocessor.hpp"

namespace Zenith {

	class GlslIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		explicit GlslIncluder(Utils::IncludePathManager* pathManager);

		~GlslIncluder() override;

		shaderc_include_result* GetInclude(const char* requestedPath, shaderc_include_type type, const char* requestingPath, size_t includeDepth) override;

		void ReleaseInclude(shaderc_include_result* data) override;

		std::unordered_set<IncludeData>&& GetIncludeData() { return std::move(m_includeData); }
		std::unordered_set<std::string>&& GetParsedSpecialMacros() { return std::move(m_ParsedSpecialMacros); }

	private:
		Utils::IncludePathManager* m_PathManager;
		std::unordered_set<IncludeData> m_includeData;
		std::unordered_set<std::string> m_ParsedSpecialMacros;
		std::unordered_map<std::string, HeaderCache> m_HeaderCache;
	};
}
#pragma once

#include "Zenith/Renderer/API/Vulkan/VulkanShaderUtils.hpp"
#include "Zenith/Renderer/Shader.hpp"
#include "Zenith/Utilities/StringUtils.hpp"

#include <filesystem>
#include <format>
#include <iterator>
#include <sstream>
#include <unordered_set>

enum VkShaderStageFlagBits;

namespace Zenith {
	namespace PreprocessUtils {
		template<bool RemoveHeaderGuard = true>
		bool ContainsHeaderGuard(std::string& header)
		{
			size_t pos = header.find('#');
			while (pos != std::string::npos)
			{
				const size_t endOfLine = header.find_first_of("\r\n", pos) + 1;
				auto tokens = Utils::SplitStringAndKeepDelims(header.substr(pos, endOfLine - pos));
				auto it = tokens.begin();

				if (*(++it) == "pragma")
				{
					if (*(++it) == "once")
					{
						if constexpr (RemoveHeaderGuard)
							header.erase(pos, endOfLine - pos);
						return true;
					}
				}
				pos = header.find('#', pos + 1);
			}
			return false;
		}

		// From https://wandbox.org/permlink/iXC7DWaU8Tk8jrf3 and is modified.
		enum class State : char { SlashOC, StarIC, SingleLineComment, MultiLineComment, NotAComment };

		template <typename InputIt, typename OutputIt>
		void CopyWithoutComments(InputIt first, InputIt last, OutputIt out)
		{
			State state = State::NotAComment;

			while (first != last)
			{
				switch (state)
				{
					case State::SlashOC:
						if (*first == '/') state = State::SingleLineComment;
						else if (*first == '*') state = State::MultiLineComment;
						else
						{
							state = State::NotAComment;
							*out++ = '/';
							*out++ = *first;
						}
						break;
					case State::StarIC:
						if (*first == '/') state = State::NotAComment;
						else state = State::MultiLineComment;
						break;
					case State::NotAComment:
						if (*first == '/') state = State::SlashOC;
						else *out++ = *first;
						break;
					case State::SingleLineComment:
						if (*first == '\n')
						{
							state = State::NotAComment;
							*out++ = '\n';
						}
						break;
					case State::MultiLineComment:
						if (*first == '*') state = State::StarIC;
						else if (*first == '\n') *out++ = '\n';
						break;
				}
				++first;
			}
		}
	}

	struct IncludeData
	{
		std::filesystem::path IncludedFilePath {};
		size_t IncludeDepth {};
		bool IsRelative { false };
		bool IsGuarded { false };
		uint32_t HashValue {};

		VkShaderStageFlagBits IncludedStage{};

		inline bool operator==(const IncludeData& other) const noexcept
		{
			return this->IncludedFilePath == other.IncludedFilePath && this->HashValue == other.HashValue;
		}
	};

	struct HeaderCache
	{
		std::string Source;
		uint32_t SourceHash;
		VkShaderStageFlagBits Stages;
		bool IsGuarded;
	};
}

namespace std {
	template<>
	struct hash<Zenith::IncludeData>
	{
		size_t operator()(const Zenith::IncludeData& data) const noexcept
		{
			return std::filesystem::hash_value(data.IncludedFilePath) ^ data.HashValue;
		}
	};
}

namespace Zenith {
	class ShaderPreprocessor
	{
	public:
		static VkShaderStageFlagBits PreprocessHeader(std::string& contents, bool& isGuarded, std::unordered_set<std::string>& specialMacros, const std::unordered_set<IncludeData>& includeData, const std::filesystem::path& fullPath);
		static std::map<VkShaderStageFlagBits, std::string> PreprocessShader(const std::string& source, std::unordered_set<std::string>& specialMacros);
	};

	inline VkShaderStageFlagBits ShaderPreprocessor::PreprocessHeader(std::string& contents, bool& isGuarded, std::unordered_set<std::string>& specialMacros, const std::unordered_set<IncludeData>& includeData, const std::filesystem::path& fullPath)
	{
		std::stringstream sourceStream;
		PreprocessUtils::CopyWithoutComments(contents.begin(), contents.end(), std::ostream_iterator<char>(sourceStream));
		contents = sourceStream.str();

		VkShaderStageFlagBits stagesInHeader = {};

		// Header guards are handled differently in HLSL
		isGuarded = PreprocessUtils::ContainsHeaderGuard<true>(contents);

		uint32_t stageCount = 0;
		size_t startOfShaderStage = contents.find('#', 0);

		while (startOfShaderStage != std::string::npos)
		{
			const size_t endOfLine = contents.find_first_of("\r\n", startOfShaderStage) + 1;
			// Parse stage. example: #pragma stage:vert
			auto tokens = Utils::SplitStringAndKeepDelims(contents.substr(startOfShaderStage, endOfLine - startOfShaderStage));

			uint32_t index = 0;
			// Pre-processor directives
			if (tokens[index] == "#")
			{
				++index;
				if (tokens[index] == "pragma")
				{
					++index;
					if (tokens[index] == "stage")
					{
						ZN_CORE_VERIFY(tokens[++index] == ":", "Stage pragma is invalid");

						// Skipped ':'
						const std::string_view stage(tokens[++index]);
						ZN_CORE_VERIFY(stage == "vert" || stage == "frag" || stage == "comp", "Invalid shader type specified");
						VkShaderStageFlagBits foundStage = ShaderUtils::StageToVKShaderStage(stage);

						const bool alreadyIncluded = std::find_if(includeData.begin(), includeData.end(), [fullPath, foundStage](const IncludeData& data)
						{
							return data.IncludedFilePath == fullPath.string() && !bool(foundStage & data.IncludedStage);
						}) != includeData.end();

						if (isGuarded && alreadyIncluded)
							contents.clear();
						else if (!isGuarded && alreadyIncluded)
							ZN_CORE_WARN("\"{}\" Header does not contain a header guard (#pragma once).", fullPath);

						// Add #endif for HLSL
						if (stageCount == 0)
							contents.replace(startOfShaderStage, endOfLine - startOfShaderStage, std::format("#ifdef {}\r\n", ShaderUtils::StageToShaderMacro(stage)));
						else // Add stage macro instead of stage pragma, both #endif and #ifdef must be in the same line, hence no '\n'
							contents.replace(startOfShaderStage, endOfLine - startOfShaderStage, std::format("#endif\r\n#ifdef {}", ShaderUtils::StageToShaderMacro(stage)));

						*(int*)&stagesInHeader |= (int)foundStage;
						stageCount++;
					}
				}
				else if (tokens[index] == "ifdef")
				{
					++index;
					if (tokens[index].rfind("__ZN_", 0) == 0) // Zenith special macros start with "__ZN_"
					{
						specialMacros.emplace(tokens[index]);
					}
				}
				else if (tokens[index] == "if" || tokens[index] == "define")
				{
					++index;
					for (size_t i = index; i < tokens.size(); ++i)
					{
						if (tokens[i].rfind("__ZN_", 0) == 0) // Zenith special macros start with "__ZN_"
						{
							specialMacros.emplace(tokens[i]);
						}
					}
				}
			}

			startOfShaderStage = contents.find('#', startOfShaderStage + 1);
		}
		if (stageCount)
			contents.append("\n#endif");
		else
		{
			const bool alreadyIncluded = std::find_if(includeData.begin(), includeData.end(), [fullPath](const IncludeData& data)
			{
				return data.IncludedFilePath == fullPath;
			}) != includeData.end();
			if (isGuarded && alreadyIncluded)
				contents.clear();
			else if (!isGuarded && alreadyIncluded)
				ZN_CORE_WARN("\"{}\" Header does not contain a header guard (#pragma once)", fullPath);
		}

		return stagesInHeader;
	}

	inline std::map<VkShaderStageFlagBits, std::string> ShaderPreprocessor::PreprocessShader(const std::string& source, std::unordered_set<std::string>& specialMacros)
	{
		std::stringstream sourceStream;
		PreprocessUtils::CopyWithoutComments(source.begin(), source.end(), std::ostream_iterator<char>(sourceStream));
		std::string newSource = sourceStream.str();

		std::map<VkShaderStageFlagBits, std::string> shaderSources;
		std::vector<std::pair<VkShaderStageFlagBits, size_t>> stagePositions;
		ZN_CORE_ASSERT(newSource.size(), "Shader is empty!");

		size_t pos = newSource.find('#');

		while (pos != std::string::npos)
		{
			const size_t endOfLine = newSource.find_first_of("\r\n", pos) + 1;
			std::vector<std::string> tokens = Utils::SplitStringAndKeepDelims(newSource.substr(pos, endOfLine - pos));

			size_t index = 1; // Skip #

			if (index < tokens.size() && tokens[index] == "pragma") // Parse stage. example: #pragma stage : vert
			{
				++index;
				if (index < tokens.size() && tokens[index] == "stage")
				{
					++index;
					// Jump over ':'
					ZN_CORE_VERIFY(index < tokens.size() && tokens[index] == ":", "Stage pragma is invalid");
					++index;

					if (index < tokens.size())
					{
						const std::string_view stage = tokens[index];
						ZN_CORE_VERIFY(stage == "vert" || stage == "frag" || stage == "comp", "Invalid shader type specified");
						auto shaderStage = ShaderUtils::ShaderTypeFromString(stage);

						// Use the position of this pragma as the start of the stage
						stagePositions.emplace_back(shaderStage, pos);
					}
				}
			}
			else if (index < tokens.size() && tokens[index] == "ifdef")
			{
				++index;
				if (index < tokens.size() && tokens[index].rfind("__ZN_", 0) == 0) // Zenith special macros start with "__ZN_"
				{
					specialMacros.emplace(tokens[index]);
				}
			}
			else if (index < tokens.size() && (tokens[index] == "if" || tokens[index] == "define"))
			{
				++index;
				for (size_t i = index; i < tokens.size(); ++i)
				{
					if (tokens[i].rfind("__ZN_", 0) == 0) // Zenith special macros start with "__ZN_"
					{
						specialMacros.emplace(tokens[i]);
					}
				}
			}

			pos = newSource.find('#', pos + 1);
		}

		ZN_CORE_VERIFY(stagePositions.size(), "Could not pre-process shader! There are no known stages defined in file.");

		// Extract common section (everything before first pragma)
		std::string commonSection;
		if (!stagePositions.empty())
		{
			commonSection = newSource.substr(0, stagePositions[0].second);
		}

		for (size_t i = 0; i < stagePositions.size(); ++i)
		{
			auto& [stage, stagePos] = stagePositions[i];

			std::string stageSpecificSource;
			if (i == stagePositions.size() - 1)
			{
				stageSpecificSource = newSource.substr(stagePos);
			}
			else
			{
				size_t nextStagePos = stagePositions[i + 1].second;
				stageSpecificSource = newSource.substr(stagePos, nextStagePos - stagePos);
			}

			size_t firstNewline = stageSpecificSource.find('\n');
			if (firstNewline != std::string::npos)
			{
				stageSpecificSource = stageSpecificSource.substr(firstNewline + 1);
			}

			std::string fullStageSource = commonSection + stageSpecificSource;
			shaderSources[stage] = fullStageSource;
		}

		return shaderSources;
	}
}
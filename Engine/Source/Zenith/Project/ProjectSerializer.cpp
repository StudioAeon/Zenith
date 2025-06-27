#include "znpch.hpp"
#include "ProjectSerializer.hpp"

#include "Zenith/Utilities/JSONSerializationHelpers.hpp"
#include "Zenith/Utilities/SerializationMacros.hpp"
#include "Zenith/Utilities/StringUtils.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

namespace Zenith {

	ProjectSerializer::ProjectSerializer(Ref<Project> project)
	   : m_Project(project)
	{
	}

	void ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		nlohmann::json root;
		nlohmann::json projectNode;

		projectNode["Name"] = m_Project->m_Config.Name;
		projectNode["AssetDirectory"] = m_Project->m_Config.AssetDirectory;
		projectNode["AssetRegistry"] = m_Project->m_Config.AssetRegistryPath;
		projectNode["StartScene"] = m_Project->m_Config.StartScene;
		projectNode["AutoSave"] = m_Project->m_Config.EnableAutoSave;
		projectNode["AutoSaveInterval"] = m_Project->m_Config.AutoSaveIntervalSeconds;

		nlohmann::json logNode;
		auto& tags = Log::EnabledTags();
		for (auto& [name, details] : tags)
		{
			if (!name.empty()) // Don't serialize untagged log
			{
				nlohmann::json tagNode;
				tagNode["Enabled"] = details.Enabled;
				tagNode["LevelFilter"] = Log::LevelToString(details.LevelFilter);
				logNode[name] = tagNode;
			}
		}
		projectNode["Log"] = logNode;

		root["Project"] = projectNode;

		std::ofstream fout(filepath);
		fout << root.dump(4);
		fout.close();

		m_Project->OnSerialized();
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			ZN_CORE_ERROR("Failed to open project file: {}", filepath.string());
			return false;
		}

		nlohmann::json root;
		try
		{
			stream >> root;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			ZN_CORE_ERROR("Failed to parse project JSON: {}", e.what());
			return false;
		}

		if (!root.contains("Project"))
		{
			ZN_CORE_ERROR("Project file missing 'Project' node");
			return false;
		}

		auto projectNode = root["Project"];
		if (!projectNode.contains("Name"))
		{
			ZN_CORE_ERROR("Project file missing 'Name' field");
			return false;
		}

		auto& config = m_Project->m_Config;

		config.Name = projectNode["Name"].get<std::string>();

		config.AssetDirectory = projectNode.value("AssetDirectory", std::string(""));
		config.AssetRegistryPath = projectNode.value("AssetRegistry", std::string(""));
		config.StartScene = projectNode.value("StartScene", std::string(""));
		config.EnableAutoSave = projectNode.value("AutoSave", false);
		config.AutoSaveIntervalSeconds = projectNode.value("AutoSaveInterval", 300);

		std::filesystem::path projectPath = filepath;
		config.ProjectFileName = projectPath.filename().string();
		config.ProjectDirectory = projectPath.parent_path().string();

		if (projectNode.contains("Log") && projectNode["Log"].is_object())
		{
			try
			{
				auto logNode = projectNode["Log"];
				auto& tags = Log::EnabledTags();

				for (auto& [name, tagData] : logNode.items())
				{
					if (tagData.is_object())
					{
						auto& details = tags[name];
						details.Enabled = tagData.value("Enabled", true);

						std::string levelStr = tagData.value("LevelFilter", "Info");
						details.LevelFilter = Log::LevelFromString(levelStr);
					}
				}
			}
			catch (const std::exception& e)
			{
				ZN_CORE_WARN("Failed to deserialize log settings: {}", e.what());
			}
		}

		m_Project->OnDeserialized();
		return true;
	}

}
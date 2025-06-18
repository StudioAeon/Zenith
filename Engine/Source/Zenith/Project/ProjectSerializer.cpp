#include "znpch.hpp"
#include "ProjectSerializer.hpp"

#include "Zenith/Utilities/JSONSerializationHelpers.hpp"

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
		nlohmann::json j;
		nlohmann::json projectNode;

		projectNode["Name"] = m_Project->m_Config.Name;
		projectNode["StartScene"] = m_Project->m_Config.StartScene;
		projectNode["AutoSave"] = m_Project->m_Config.EnableAutoSave;
		projectNode["AutoSaveInterval"] = m_Project->m_Config.AutoSaveIntervalSeconds;

		// Log settings
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

		// Root node
		j["Project"] = projectNode;

		std::ofstream fout(filepath);
		fout << j.dump(4);
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

		nlohmann::json j;
		try
		{
			stream >> j;
		}
		catch (nlohmann::json::parse_error& e)
		{
			ZN_CORE_ERROR("Failed to parse project file: {}", e.what());
			return false;
		}

		if (!j.contains("Project"))
		{
			ZN_CORE_ERROR("Project file missing 'Project' node");
			return false;
		}

		auto projectNode = j["Project"];
		if (!projectNode.contains("Name"))
		{
			ZN_CORE_ERROR("Project file missing 'Name' field");
			return false;
		}

		auto& config = m_Project->m_Config;

		// Clear existing config to ensure clean state
		config = ProjectConfig{};

		// Safely extract strings with fallbacks
		try
		{
			config.Name = projectNode["Name"].get<std::string>();
			if (config.Name.empty())
				config.Name = "Unnamed Project";
		}
		catch (...)
		{
			ZN_CORE_ERROR("Failed to read project name");
			config.Name = "Unnamed Project";
		}

		std::filesystem::path projectPath = filepath;
		config.ProjectFileName = projectPath.filename().string();
		config.ProjectDirectory = projectPath.parent_path().string();

		try
		{
			config.StartScene = projectNode.value("StartScene", std::string(""));
		}
		catch (...)
		{
			config.StartScene = "";
		}

		config.EnableAutoSave = projectNode.value("AutoSave", false);
		config.AutoSaveIntervalSeconds = projectNode.value("AutoSaveInterval", 300);

		// Log settings
		if (projectNode.contains("Log"))
		{
			try
			{
				auto logNode = projectNode["Log"];
				auto& tags = Log::EnabledTags();

				for (auto& [name, tagData] : logNode.items())
				{
					auto& details = tags[name];
					details.Enabled = tagData.value("Enabled", true);

					std::string levelStr = tagData.value("LevelFilter", "Info");
					details.LevelFilter = Log::LevelFromString(levelStr);
				}
			}
			catch (...)
			{
				ZN_CORE_WARN("Failed to deserialize log settings");
			}
		}

		ZN_CORE_INFO("Successfully deserialized project: {}", config.Name);
		m_Project->OnDeserialized();
		return true;
	}

}
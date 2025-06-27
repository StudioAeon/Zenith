#include "znpch.hpp"
#include "UserPreferences.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

namespace Zenith {

	UserPreferencesSerializer::UserPreferencesSerializer(const Ref<UserPreferences>& preferences)
		: m_Preferences(preferences)
	{
	}

	UserPreferencesSerializer::~UserPreferencesSerializer()
	{
	}

	void UserPreferencesSerializer::Serialize(const std::filesystem::path& filepath)
	{
		nlohmann::json j;
		nlohmann::json userPrefsNode;

		if (!m_Preferences->StartupProject.empty())
		{
			userPrefsNode["StartupProject"] = m_Preferences->StartupProject;
		}

		nlohmann::json recentProjectsArray = nlohmann::json::array();
		for (const auto& [lastOpened, projectConfig] : m_Preferences->RecentProjects)
		{
			nlohmann::json projectNode;
			projectNode["Name"] = projectConfig.Name;
			projectNode["ProjectPath"] = projectConfig.FilePath;
			projectNode["LastOpened"] = projectConfig.LastOpened;
			recentProjectsArray.push_back(projectNode);
		}
		userPrefsNode["RecentProjects"] = recentProjectsArray;

		// Root node
		j["UserPrefs"] = userPrefsNode;

		std::ofstream fout(filepath);
		if (!fout.is_open())
		{
			ZN_CORE_ERROR("Failed to open user preferences file for writing: {}", filepath.string());
			return;
		}

		fout << j.dump(4);
		fout.close();

		m_Preferences->FilePath = filepath.string();
	}

	void UserPreferencesSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			ZN_CORE_WARN("Failed to open user preferences file: {}", filepath.string());
			return;
		}

		nlohmann::json j;
		try
		{
			stream >> j;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			ZN_CORE_ERROR("Failed to parse user preferences file: {}", e.what());
			stream.close();
			return;
		}

		stream.close();

		if (!j.contains("UserPrefs"))
		{
			ZN_CORE_WARN("User preferences file missing 'UserPrefs' node");
			return;
		}

		nlohmann::json userPrefsNode = j["UserPrefs"];

		if (userPrefsNode.contains("StartupProject") && userPrefsNode["StartupProject"].is_string())
		{
			m_Preferences->StartupProject = userPrefsNode["StartupProject"].get<std::string>();
		}
		else
		{
			m_Preferences->StartupProject = "";
		}

		m_Preferences->RecentProjects.clear();

		if (userPrefsNode.contains("RecentProjects") && userPrefsNode["RecentProjects"].is_array())
		{
			for (const auto& recentProjectNode : userPrefsNode["RecentProjects"])
			{
				try
				{
					RecentProject entry;

					if (recentProjectNode.contains("Name") && recentProjectNode["Name"].is_string())
						entry.Name = recentProjectNode["Name"].get<std::string>();
					else
						entry.Name = "Unknown Project";

					if (recentProjectNode.contains("ProjectPath") && recentProjectNode["ProjectPath"].is_string())
						entry.FilePath = recentProjectNode["ProjectPath"].get<std::string>();
					else
						continue; // Skip entries without valid path

					if (recentProjectNode.contains("LastOpened"))
					{
						if (recentProjectNode["LastOpened"].is_number_integer())
							entry.LastOpened = recentProjectNode["LastOpened"].get<time_t>();
						else
							entry.LastOpened = time(nullptr);
					}
					else
					{
						entry.LastOpened = time(nullptr);
					}

					m_Preferences->RecentProjects[entry.LastOpened] = entry;
				}
				catch (const std::exception& e)
				{
					ZN_CORE_WARN("Failed to parse recent project entry: {}", e.what());
					continue; // Skip malformed entries
				}
			}
		}

		m_Preferences->FilePath = filepath.string();
		ZN_CORE_INFO("User preferences loaded from: {}", filepath.string());
	}

}
#pragma once

#include "Zenith/Core/Log.hpp"
#include "Zenith/Project/Project.hpp"

#include <map>

namespace Zenith {

	struct RecentProject
	{
		std::string Name;
		std::string FilePath;
		time_t LastOpened;
	};

	struct UserPreferences : public RefCounted
	{
		std::string StartupProject;
		std::map<time_t, RecentProject, std::greater<time_t>> RecentProjects;

		// Not Serialized
		std::string FilePath;
	};

	class UserPreferencesSerializer
	{
	public:
		UserPreferencesSerializer(const Ref<UserPreferences>& preferences);
		~UserPreferencesSerializer();

		void Serialize(const std::filesystem::path& filepath);
		void Deserialize(const std::filesystem::path& filepath);

	private:
		Ref<UserPreferences> m_Preferences;
	};

}
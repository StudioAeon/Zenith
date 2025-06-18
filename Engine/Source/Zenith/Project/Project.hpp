#pragma once

#include "Zenith/Core/Assert.hpp"
#include "Zenith/Core/Log.hpp"
#include "Zenith/Core/Ref.hpp"

#include <filesystem>
#include <format>

namespace Zenith {

	struct ProjectConfig
	{
		std::string Name;

		std::string StartScene;

		bool EnableAutoSave = false;
		int AutoSaveIntervalSeconds = 300;

		std::string ProjectFileName;
		std::string ProjectDirectory;
	};

	class Project : public RefCounted
	{
	public:
		Project();
		~Project();

		const ProjectConfig& GetConfig() const { return m_Config; }

		static Ref<Project> GetActive() { return s_ActiveProject; }
		static void SetActive(Ref<Project> project);

		static const std::string& GetProjectName()
		{
			ZN_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetConfig().Name;
		}

		static std::filesystem::path GetProjectDirectory()
		{
			ZN_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetConfig().ProjectDirectory;
		}

		static std::filesystem::path GetCacheDirectory()
		{
			ZN_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / "Cache";
		}
	private:
		void OnSerialized();
		void OnDeserialized();

	private:
		ProjectConfig m_Config;

		friend class ProjectSerializer;

		inline static Ref<Project> s_ActiveProject;
	};

}

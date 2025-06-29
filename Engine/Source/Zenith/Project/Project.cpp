#include "znpch.hpp"
#include "Project.hpp"

#include "Zenith/Asset/AssetManager.hpp"

namespace Zenith {

	Project::Project()
	{}

	Project::~Project()
	{}

	void Project::SetActive(Ref<Project> project, ApplicationContext* context)
	{
		if (s_ActiveProject)
		{
			s_AssetManager->Shutdown();
			s_AssetManager = nullptr;
		}

		s_ActiveProject = project;
		if (s_ActiveProject && context)
		{
			s_AssetManager = Ref<EditorAssetManager>::Create(*context);
		}
	}

	void Project::OnSerialized()
	{}

	void Project::OnDeserialized()
	{}

}

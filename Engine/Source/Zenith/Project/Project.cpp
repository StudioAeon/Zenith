#include "znpch.hpp"
#include "Project.hpp"

namespace Zenith {

	Project::Project()
	{}

	Project::~Project()
	{}

	void Project::SetActive(Ref<Project> project)
	{
		if (s_ActiveProject)
		{}

		s_ActiveProject = project;
		if (s_ActiveProject)
		{}
	}

	void Project::OnSerialized()
	{}

	void Project::OnDeserialized()
	{}

}

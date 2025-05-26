#include "EditorLayer.hpp"

namespace Zenith {

	EditorLayer::EditorLayer()
	{}

	EditorLayer::~EditorLayer()
	{}

	void EditorLayer::OnAttach()
	{
		UpdateWindowTitle("Untitled Project");
	}

	void EditorLayer::OnDetach()
	{}

	void EditorLayer::UpdateWindowTitle(const std::string& sceneName)
	{
		std::string title = sceneName + " - Zenith-Editor - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ")";
		Application::Get().GetWindow().SetTitle(title);
	}

	void EditorLayer::OnUpdate()
	{}

	void EditorLayer::OnEvent(Event& e)
	{}


}
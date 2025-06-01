#include "EditorLayer.hpp"

#include <iostream>

namespace Zenith {

	EditorLayer::EditorLayer()
	{}

	EditorLayer::~EditorLayer()
	{}

	void EditorLayer::OnAttach()
	{
		UpdateWindowTitle("Untitled Project");

		auto uuid = Zenith::UUID::generate();
		ZN_INFO("Generated UUID: {}", uuid);
	}

	void EditorLayer::OnDetach()
	{}

	void EditorLayer::UpdateWindowTitle(const std::string& sceneName)
	{
		std::string title = sceneName + " - Zenith-Editor - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ")";
		Application::Get().GetWindow().SetTitle(title);
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{}

	void EditorLayer::OnEvent(Event& e)
	{}


}
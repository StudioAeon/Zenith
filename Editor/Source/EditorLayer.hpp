#pragma once

#include "Zenith.hpp"

#include "Zenith/ImGui/ImGuiLayer.hpp"

#include <string>

#include "Zenith/Project/UserPreferences.hpp"

#include <future>

namespace Zenith {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer(const Ref<UserPreferences>& userPreferences);
		virtual ~EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;

		virtual void OnImGuiRender() override;
		virtual bool OnEvent(Event& e) override;

		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void OpenProject();
		void OpenProject(const std::filesystem::path& filepath);

		void CreateProject(std::filesystem::path projectPath);
		void EmptyProject();
		void UpdateCurrentProject();
		void SaveProject();
		void CloseProject(bool unloadProject = true);
	private:
		std::string m_ProjectNameBuffer;
		std::string m_OpenProjectFilePathBuffer;
		std::string m_NewProjectFilePathBuffer;

		Ref<UserPreferences> m_UserPreferences;

	private:
		void UpdateWindowTitle(const std::string& sceneName);
	};

}
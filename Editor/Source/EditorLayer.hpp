#pragma once

#include "Zenith.hpp"

#include "Zenith/ImGui/ImGuiLayer.hpp"

#include <string>

#include "Zenith/Project/UserPreferences.hpp"

#include "Zenith/Renderer/Mesh.hpp"
#include "Zenith/Asset/AssetManager.hpp"

#include <future>
#include "Zenith/Editor/EditorCamera.hpp"

#include "Zenith/Renderer/MeshRenderer.hpp"
#include <memory>

namespace Zenith {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer(Ref<UserPreferences> userPreferences);
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		virtual bool OnEvent(Event& e) override;

		void SetApplicationContext(std::shared_ptr<ApplicationContext> context);

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
		void TestLoadMesh();
		void RenderMeshTestUI();
		void RenderCameraControlsUI();
		void UpdateViewportBounds();

		void RenderMeshInspector();
		void RenderMaterialInspector();
	private:
		std::string m_ProjectNameBuffer;
		std::string m_OpenProjectFilePathBuffer;
		std::string m_NewProjectFilePathBuffer;

		Ref<UserPreferences> m_UserPreferences;

		std::shared_ptr<ApplicationContext> m_ApplicationContext;

		Ref<MeshSource> m_TestMeshSource;
		std::string m_MeshTestLog;
		bool m_MeshLoadSuccess = false;
		uint32_t m_LoadedVertexCount = 0;
		uint32_t m_LoadedIndexCount = 0;
		uint32_t m_LoadedSubmeshCount = 0;
		std::unique_ptr<MeshRenderer> m_MeshRenderer;
		glm::mat4 m_MeshTransform = glm::mat4(1.0f);
		float m_MeshRotation = 0.0f;
		bool m_EnableMeshRendering = false;

		Ref<EditorCamera> m_EditorCamera;
		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;
		ImVec2 m_ViewportSize = {0, 0};
		ImVec2 m_ViewportBounds[2];

		bool m_UseIdentityTransform = false;
		bool m_UseUnreversedProjection = true;
		bool m_ForceCameraActive = false;

	private:
		void UpdateWindowTitle(const std::string& sceneName);
	};

}

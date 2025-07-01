#include "EditorLayer.hpp"
#include "Zenith/Core/ApplicationContext.hpp"

#include "Zenith/Debug/Profiler.hpp"

#include "Zenith/Project/Project.hpp"
#include "Zenith/Project/ProjectSerializer.hpp"
#include "Zenith/Utilities/FileSystem.hpp"
#include "Zenith/Utilities/StringUtils.hpp"

#include <imgui/imgui_internal.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <future>

#include "Zenith/Renderer/MeshRenderer.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include "Zenith/Asset/MeshImporter.hpp"

namespace Zenith {

	EditorLayer::EditorLayer(Ref<UserPreferences> userPreferences)
		: Layer("EditorLayer"), m_UserPreferences(userPreferences), m_ApplicationContext(nullptr)
	{
		m_ProjectNameBuffer.reserve(255);
		m_OpenProjectFilePathBuffer.reserve(512);
		m_NewProjectFilePathBuffer.reserve(512);

		for (auto it = m_UserPreferences->RecentProjects.begin(); it != m_UserPreferences->RecentProjects.end(); )
		{
			if (!std::filesystem::exists(it->second.FilePath))
				it = m_UserPreferences->RecentProjects.erase(it);
			else
				it++;
		}
	}

	void EditorLayer::OnAttach()
	{
		UpdateWindowTitle("Project Apex");

		if (!m_UserPreferences->StartupProject.empty())
			OpenProject(m_UserPreferences->StartupProject);
		else
			ZN_VERIFY(false, "No project provided!");

		if (!Project::GetActive())
			EmptyProject();

		m_MeshRenderer = std::make_unique<MeshRenderer>();
		m_MeshRenderer->Initialize();

		m_EditorCamera = Ref<EditorCamera>::Create(45.0f, 1920.0f, 1080.0f, 0.1f, 1000.0f);
		m_EditorCamera->SetActive(false);

		m_EditorCamera->Focus(glm::vec3(0.0f, 8.0f, 0.0f));
		m_EditorCamera->SetDistance(5.0f);

		TestLoadMesh();
		if (m_MeshLoadSuccess)
		{
			m_EnableMeshRendering = true;
		}
	}

	void EditorLayer::SetApplicationContext(std::shared_ptr<ApplicationContext> context)
	{
		m_ApplicationContext = context;
	}

	void EditorLayer::OnDetach()
	{
		CloseProject(true);
		Project::SetActive(nullptr, nullptr);

		if (m_MeshRenderer)
		{
			m_MeshRenderer->Shutdown();
			m_MeshRenderer.reset();
		}

		m_EditorCamera = nullptr;
	}

	void EditorLayer::UpdateWindowTitle(const std::string& sceneName)
	{
		std::string title = sceneName + " - Zenith-Editor - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ")";

		if (m_ApplicationContext)
		{
			m_ApplicationContext->GetWindow().SetTitle(title);
		}
		else
		{
			ZN_WARN("EditorLayer: No application context available for window title update");
		}
	}

	static void ReplaceToken(std::string& str, std::string_view token, const std::string& value)
	{
		size_t pos = 0;
		while ((pos = str.find(token, pos)) != std::string::npos)
		{
			str.replace(pos, token.length(), value);
			pos += token.length();
		}
	}

	void EditorLayer::CreateProject(std::filesystem::path projectPath)
	{
		if (!std::filesystem::exists(projectPath))
			std::filesystem::create_directories(projectPath);

		std::filesystem::copy("Resources/NewProjectTemplate", projectPath, std::filesystem::copy_options::recursive);

		{
			std::ifstream stream(projectPath / "Project.zproj");
			ZN_VERIFY(stream.is_open());
			std::stringstream ss;
			ss << stream.rdbuf();
			stream.close();

			std::string str = ss.str();
			ReplaceToken(str, "$PROJECT_NAME$", m_ProjectNameBuffer);

			std::ofstream ostream(projectPath / "Project.zproj");
			ostream << str;
			ostream.close();

			std::string newProjectFileName = m_ProjectNameBuffer + ".zproj";
			std::filesystem::rename(projectPath / "Project.zproj", projectPath / newProjectFileName);
		}

		{
			RecentProject projectEntry;
			projectEntry.Name = m_ProjectNameBuffer;
			projectEntry.FilePath = (projectPath / (m_ProjectNameBuffer + ".zproj")).string();
			projectEntry.LastOpened = time(NULL);
			m_UserPreferences->RecentProjects[projectEntry.LastOpened] = projectEntry;

			UserPreferencesSerializer preferencesSerializer(m_UserPreferences);
			preferencesSerializer.Serialize(m_UserPreferences->FilePath);
		}

		Log::SetDefaultTagSettings();
		OpenProject(projectPath / (m_ProjectNameBuffer + ".zproj"));

		SaveProject();
	}

	void EditorLayer::EmptyProject()
	{
		if (Project::GetActive())
			CloseProject();

		Ref<Project> project = Ref<Project>::Create();
		Project::SetActive(project, m_ApplicationContext.get());

		m_ProjectNameBuffer.clear();
		m_OpenProjectFilePathBuffer.clear();
		m_NewProjectFilePathBuffer.clear();
	}

	void EditorLayer::UpdateCurrentProject()
	{}

	void EditorLayer::OpenProject()
	{
		std::filesystem::path filepath = FileSystem::OpenFileDialog({ { "Zenith Project", "zproj" } });

		if (filepath.empty())
			return;

		m_OpenProjectFilePathBuffer = filepath.string();

		RecentProject projectEntry;
		projectEntry.Name = Utils::RemoveExtension(filepath.filename().string());
		projectEntry.FilePath = filepath.string();
		projectEntry.LastOpened = time(NULL);

		for (auto it = m_UserPreferences->RecentProjects.begin(); it != m_UserPreferences->RecentProjects.end(); it++)
		{
			if (it->second.Name == projectEntry.Name)
			{
				m_UserPreferences->RecentProjects.erase(it);
				break;
			}
		}

		m_UserPreferences->RecentProjects[projectEntry.LastOpened] = projectEntry;

		UserPreferencesSerializer serializer(m_UserPreferences);
		serializer.Serialize(m_UserPreferences->FilePath);
	}

	void EditorLayer::OpenProject(const std::filesystem::path& filepath)
	{
		if (!FileSystem::Exists(filepath))
		{
			ZN_ERROR("Tried to open a project that doesn't exist. Project path: {0}", filepath);
			m_OpenProjectFilePathBuffer.clear();
			return;
		}

		if (Project::GetActive())
			CloseProject();

		Ref<Project> project = Ref<Project>::Create();
		ProjectSerializer serializer(project);
		serializer.Deserialize(filepath);

		Project::SetActive(project, m_ApplicationContext.get());

		m_ProjectNameBuffer.clear();
		m_OpenProjectFilePathBuffer.clear();
		m_NewProjectFilePathBuffer.clear();
	}

	void EditorLayer::SaveProject()
	{
		if (!Project::GetActive())
			ZN_VERIFY(false); // TODO

		auto project = Project::GetActive();
		ProjectSerializer serializer(project);
		serializer.Serialize(project->GetConfig().ProjectDirectory + "/" + project->GetConfig().ProjectFileName);
	}

	void EditorLayer::CloseProject(bool unloadProject)
	{
		if (Project::GetActive())
			SaveProject();

		if (unloadProject)
			Project::SetActive(nullptr);
	}

	void EditorLayer::TestLoadMesh()
	{
		m_TestMeshSource = nullptr;
		m_MeshLoadSuccess = false;
		m_LoadedVertexCount = 0;
		m_LoadedIndexCount = 0;
		m_LoadedSubmeshCount = 0;
		m_MeshTestLog.clear();

		std::filesystem::path meshPath = "ProjectApex/Assets/Meshes/Gltf/Sponza/Sponza.glb";

		if (!std::filesystem::exists(meshPath))
		{
			m_MeshTestLog = "ERROR: " + meshPath.filename().string() + " not found at " + meshPath.string();
			ZN_ERROR("{}", m_MeshTestLog);
			return;
		}

		try
		{
			MeshImporter importer(meshPath);
			m_TestMeshSource = importer.ImportToMeshSource();

			if (m_TestMeshSource)
			{
				m_MeshLoadSuccess = true;
				m_LoadedVertexCount = static_cast<uint32_t>(m_TestMeshSource->GetVertices().size());
				m_LoadedIndexCount = static_cast<uint32_t>(m_TestMeshSource->GetIndices().size());
				m_LoadedSubmeshCount = static_cast<uint32_t>(m_TestMeshSource->GetSubmeshes().size());

				const auto& boundingBox = m_TestMeshSource->GetBoundingBox();
				m_MeshTestLog = std::format(
					"SUCCESS: Loaded {} successfully!\n"
					"- Vertices: {}\n"
					"- Indices: {}\n"
					"- Submeshes: {}\n"
					"- Materials: {}\n"
					"- Bounding Box: Min({:.2f}, {:.2f}, {:.2f}) Max({:.2f}, {:.2f}, {:.2f})",
					meshPath.filename().string(),
					m_LoadedVertexCount,
					m_LoadedIndexCount,
					m_LoadedSubmeshCount,
					m_TestMeshSource->GetMaterials().size(),
					boundingBox.Min.x, boundingBox.Min.y, boundingBox.Min.z,
					boundingBox.Max.x, boundingBox.Max.y, boundingBox.Max.z
				);
			}
			else
			{
				m_MeshTestLog = "ERROR: Failed to import mesh - importer returned null";
				ZN_ERROR("{}", m_MeshTestLog);
			}
		}
		catch (const std::exception& e)
		{
			m_MeshTestLog = std::format("EXCEPTION: {}", e.what());
			ZN_ERROR("Exception during mesh loading: {}", e.what());
		}
	}

	void EditorLayer::UpdateViewportBounds()
	{
		if (m_EditorCamera && m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
		{
			uint32_t left = static_cast<uint32_t>(m_ViewportBounds[0].x);
			uint32_t top = static_cast<uint32_t>(m_ViewportBounds[0].y);
			uint32_t right = static_cast<uint32_t>(m_ViewportBounds[1].x);
			uint32_t bottom = static_cast<uint32_t>(m_ViewportBounds[1].y);

			m_EditorCamera->SetViewportBounds(left, top, right, bottom);
		}
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		ZN_PROFILE_FUNC();

		AssetManager::SyncWithAssetThread();

		if (m_EditorCamera)
		{
			bool shouldActivateCamera = m_ViewportFocused;
			m_EditorCamera->SetActive(shouldActivateCamera);
			m_EditorCamera->OnUpdate(ts);
		}

		if (m_EnableMeshRendering && m_TestMeshSource && m_MeshRenderer && m_EditorCamera)
		{
			m_MeshTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));

			glm::mat4 viewProjection = m_EditorCamera->GetUnReversedViewProjection();

			m_MeshRenderer->BeginScene(viewProjection);
			m_MeshRenderer->DrawMesh(m_TestMeshSource, m_MeshTransform);
			m_MeshRenderer->EndScene();
		}
	}

	void EditorLayer::RenderCameraControlsUI()
	{
		if (!m_EditorCamera)
			return;

		if (ImGui::Begin("Camera Controls"))
		{
			ImGui::SeparatorText("Camera Info");

			glm::vec3 position = m_EditorCamera->GetPosition();
			ImGui::Text("Position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);
			ImGui::Text("Distance: %.2f", m_EditorCamera->GetDistance());

			float yaw = glm::degrees(m_EditorCamera->GetYaw());
			float pitch = glm::degrees(m_EditorCamera->GetPitch());
			ImGui::Text("Rotation: Yaw %.1f°, Pitch %.1f°", yaw, pitch);
			ImGui::Text("FOV: %.1f°", glm::degrees(m_EditorCamera->GetVerticalFOV()));
			ImGui::Text("Mode: %s",
				m_EditorCamera->GetCurrentMode() == CameraMode::FLYCAM ? "Fly Camera" :
				m_EditorCamera->GetCurrentMode() == CameraMode::ARCBALL ? "Arc Ball" : "None");

			ImGui::SeparatorText("Controls");
			ImGui::TextWrapped("Fly Camera: Right mouse + WASD/QE");
			ImGui::TextWrapped("Arc Ball: Alt + Left mouse (rotate), Middle mouse (pan), Right mouse (zoom)");
			ImGui::TextWrapped("Scroll: Zoom in/out");

			ImGui::SeparatorText("Camera Settings");

			if (ImGui::Button("Focus on Origin"))
			{
				m_EditorCamera->Focus(glm::vec3(0.0f));
			}

			ImGui::SameLine();
			if (ImGui::Button("View Mesh"))
			{
				m_EditorCamera->Focus(glm::vec3(0.0f, 2.0f, 0.0f));
				m_EditorCamera->SetDistance(25.0f);
			}

			float distance = m_EditorCamera->GetDistance();
			if (ImGui::SliderFloat("Distance", &distance, 1.0f, 100.0f))
			{
				ZN_CORE_INFO("UI Setting distance to: {}", distance);
				m_EditorCamera->SetDistance(distance);
			}

			ImGui::Separator();
			ImGui::Text("Viewport Size: %.0fx%.0f", m_ViewportSize.x, m_ViewportSize.y);

			if (m_TestMeshSource)
			{
				const auto& bounds = m_TestMeshSource->GetBoundingBox();
				ImGui::Separator();
				ImGui::Text("Mesh Info:");
				ImGui::Text("Width: %.1f units", bounds.Max.x - bounds.Min.x);
				ImGui::Text("Height: %.1f units", bounds.Max.y - bounds.Min.y);
				ImGui::Text("Depth: %.1f units", bounds.Max.z - bounds.Min.z);
			}
		}
		ImGui::End();
	}

	void EditorLayer::RenderMeshTestUI()
	{
		if (ImGui::Begin("Mesh Renderer Test"))
		{
			ImGui::SeparatorText("Mesh Loading");
			if (ImGui::Button("Load Mesh", ImVec2(-1, 30)))
			{
				TestLoadMesh();
			}

			ImGui::Separator();

			if (m_MeshLoadSuccess)
			{
				ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Mesh Loaded Successfully");
				ImGui::Text("Vertices: %u", m_LoadedVertexCount);
				ImGui::Text("Indices: %u", m_LoadedIndexCount);
				ImGui::Text("Submeshes: %u", m_LoadedSubmeshCount);

				ImGui::Separator();
				ImGui::SeparatorText("Rendering");

				ImGui::Checkbox("Enable Mesh Rendering", &m_EnableMeshRendering);
			}
			else if (!m_MeshTestLog.empty())
			{
				ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Load Failed");
			}
			else
			{
				ImGui::TextDisabled("No mesh loaded");
			}

			if (!m_MeshTestLog.empty())
			{
				ImGui::Separator();
				ImGui::TextWrapped("%s", m_MeshTestLog.c_str());
			}
		}
		ImGui::End();
	}

	void EditorLayer::OnImGuiRender()
	{
		ZN_PROFILE_FUNC();

		// ImGui + Dockspace Setup
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		auto boldFont = io.Fonts->Fonts[0];
		auto largeFont = io.Fonts->Fonts[1];

		io.ConfigWindowsResizeFromEdges = io.BackendFlags & ImGuiBackendFlags_HasMouseCursors;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		auto& window = m_ApplicationContext->GetWindow();
		bool isMaximized = (SDL_GetWindowFlags(window.GetNativeWindow()) & SDL_WINDOW_MAXIMIZED) != 0;

#ifdef ZN_PLATFORM_WINDOWS
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);
#else
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
#endif

		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui::Begin("DockSpace Demo", nullptr, window_flags);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		ImGui::PopStyleVar(2);

		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		ImGui::DockSpace(ImGui::GetID("MyDockspace"));
		style.WindowMinSize.x = minWinSizeX;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		if (ImGui::Begin("Viewport"))
		{
			m_ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
			m_ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

			if (viewportPanelSize.x != m_ViewportSize.x || viewportPanelSize.y != m_ViewportSize.y)
			{
				if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
				{
					m_ViewportSize = viewportPanelSize;

					ImVec2 windowPos = ImGui::GetWindowPos();
					ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
					ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();

					m_ViewportBounds[0] = { windowPos.x + contentRegionMin.x, windowPos.y + contentRegionMin.y };
					m_ViewportBounds[1] = { windowPos.x + contentRegionMax.x, windowPos.y + contentRegionMax.y };

					UpdateViewportBounds();
				}
			}

			if (m_MeshRenderer && m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
			{
				Ref<Image2D> renderedImage = m_MeshRenderer->GetImage(0);
				if (renderedImage)
				{
					ImTextureID textureID = m_MeshRenderer->GetTextureImGuiID(renderedImage);
					if (textureID)
					{
						ImGui::Image(textureID, m_ViewportSize);

						if (ImGui::IsItemClicked())
						{
							m_ViewportFocused = true;
						}

						if (ImGui::IsItemHovered())
						{
							m_ViewportHovered = true;
						}
					}
					else
					{
						ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Waiting for texture...");
					}
				}
				else
				{
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No rendered image available");
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Viewport (Empty)");
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::End();

		if (ImGui::Begin("Settings"))
		{
			ImGui::SeparatorText("Graphics");

			if (m_ApplicationContext)
			{
				Window& window = m_ApplicationContext->GetWindow();
				bool vsync = window.IsVSync();
				if (ImGui::Checkbox("VSync", &vsync))
					window.SetVSync(vsync);
			}

			ImGui::SeparatorText("File Operations");

			if (ImGui::Button("Open File Dialog", ImVec2(-1, 0)))
			{
				std::initializer_list<FileSystem::FileDialogFilterItem> filters = {
					{"Text Files", "txt"},
					{"Images", "png,jpg,jpeg,bmp,tga"},
				};

				std::filesystem::path filePath = FileSystem::OpenFileDialog(filters);

				if (!filePath.empty())
					ZN_INFO("Selected file: {0}", filePath.string());
				else
					ZN_INFO("No file selected or dialog cancelled.");
			}
		}
		ImGui::End();

		if (ImGui::Begin("Controller Test"))
		{
			static int controllerID_UI = 4;
			static int buttonID = 0;
			static int axisID = 0;

			ImGui::SliderInt("Controller ID", &controllerID_UI, 1, 10);
			ImGui::SliderInt("Button ID", &buttonID, 0, 15);
			ImGui::SliderInt("Axis ID", &axisID, 0, 5);

			int controllerID = controllerID_UI - 1;

			ImGui::Separator();

			if (Input::IsControllerPresent(controllerID))
			{
				ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), "Controller %d: Connected", controllerID_UI);

				bool isDown = Input::IsControllerButtonDown(controllerID, buttonID);
				float axisValue = Input::GetControllerAxis(controllerID, axisID);

				ImGui::Text("Button %d: %s", buttonID, isDown ? "Pressed" : "Released");
				ImGui::Text("Axis %d: %.3f", axisID, axisValue);

				ImGui::ProgressBar((axisValue + 1.0f) * 0.5f, ImVec2(-1, 0), "");
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Controller %d: Not Connected", controllerID_UI);
			}
		}
		ImGui::End();

		RenderMeshTestUI();
		RenderCameraControlsUI();
	}

	bool EditorLayer::OnEvent(Event& e)
	{
		if (m_EditorCamera && m_ViewportFocused)
		{
			m_EditorCamera->OnEvent(e);
		}

		return false;
	}

}

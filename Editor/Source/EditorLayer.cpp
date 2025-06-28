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
	}

	void EditorLayer::SetApplicationContext(std::shared_ptr<ApplicationContext> context)
	{
		m_ApplicationContext = context;
	}

	void EditorLayer::OnDetach()
	{
		CloseProject(true);
		Project::SetActive(nullptr);

		if (m_MeshRenderer)
		{
			m_MeshRenderer->Shutdown();
			m_MeshRenderer.reset();
		}
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

	static void ReplaceToken(std::string& str, const char* token, const std::string& value)
	{
		size_t pos = 0;
		while ((pos = str.find(token, pos)) != std::string::npos)
		{
			str.replace(pos, strlen(token), value);
			pos += strlen(token);
		}
	}

	void EditorLayer::CreateProject(std::filesystem::path projectPath)
	{
		if (!std::filesystem::exists(projectPath))
			std::filesystem::create_directories(projectPath);

		std::filesystem::copy("Resources/NewProjectTemplate", projectPath, std::filesystem::copy_options::recursive);

		{
			// Project File
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
		Project::SetActive(project);

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

		Project::SetActive(project);

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

		std::filesystem::path meshPath = "Resources/Meshes/Default/Cube.glb";

		if (!std::filesystem::exists(meshPath))
		{
			m_MeshTestLog = "ERROR: Cube.glb not found at " + meshPath.string();
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

	void EditorLayer::OnUpdate(Timestep ts)
	{
		ZN_PROFILE_FUNC();

		AssetManager::SyncWithAssetThread();

		if (m_EnableMeshRendering && m_TestMeshSource && m_MeshRenderer)
		{
			// Update mesh rotation
			m_MeshRotation += ts * 45.0f;
			m_MeshTransform = glm::rotate(glm::mat4(1.0f), glm::radians(m_MeshRotation), glm::vec3(0.0f, 1.0f, 0.0f));

			// Set up camera matrices
			float aspectRatio = m_ApplicationContext->GetWindow().GetWidth() / (float)m_ApplicationContext->GetWindow().GetHeight();

			// Use viewport size if available
			if (m_LastViewportSize.x > 0 && m_LastViewportSize.y > 0)
			{
				aspectRatio = m_LastViewportSize.x / m_LastViewportSize.y;
			}

			glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
			glm::mat4 view = glm::lookAt(
				glm::vec3(0.0f, 2.0f, 5.0f),  // Eye position
				glm::vec3(0.0f, 0.0f, 0.0f),  // Look at center
				glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
			);
			glm::mat4 viewProjection = projection * view;

			// Render the mesh to our offscreen framebuffer
			m_MeshRenderer->BeginScene(viewProjection);
			m_MeshRenderer->DrawMesh(m_TestMeshSource, m_MeshTransform);
			m_MeshRenderer->EndScene();
		}
	}

	void EditorLayer::RenderMeshTestUI()
	{
		if (ImGui::Begin("Mesh Renderer Test"))
		{
			ImGui::SeparatorText("Mesh Loading");
			if (ImGui::Button("Load Cube.glb", ImVec2(-1, 30)))
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

				if (m_EnableMeshRendering)
				{
					ImGui::DragFloat("Rotation Speed", &m_MeshRotation, 1.0f, -360.0f, 360.0f);

					static glm::vec3 translation(0.0f);
					static glm::vec3 scale(1.0f);

					if (ImGui::DragFloat3("Position", &translation.x, 0.1f))
					{
						m_MeshTransform = glm::translate(glm::mat4(1.0f), translation) *
										  glm::rotate(glm::mat4(1.0f), glm::radians(m_MeshRotation), glm::vec3(0.0f, 1.0f, 0.0f)) *
										  glm::scale(glm::mat4(1.0f), scale);
					}

					if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f))
					{
						m_MeshTransform = glm::translate(glm::mat4(1.0f), translation) *
										  glm::rotate(glm::mat4(1.0f), glm::radians(m_MeshRotation), glm::vec3(0.0f, 1.0f, 0.0f)) *
										  glm::scale(glm::mat4(1.0f), scale);
					}

					if (ImGui::Button("Reset Transform"))
					{
						translation = glm::vec3(0.0f);
						scale = glm::vec3(1.0f);
						m_MeshRotation = 0.0f;
						m_MeshTransform = glm::mat4(1.0f);
					}
				}
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

		// ImGui + Dockspace Setup ------------------------------------------------------------------------------
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		auto boldFont = io.Fonts->Fonts[0];
		auto largeFont = io.Fonts->Fonts[1];

		io.ConfigWindowsResizeFromEdges = io.BackendFlags & ImGuiBackendFlags_HasMouseCursors;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
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
		ImGui::PopStyleColor(); // MenuBarBg
		ImGui::PopStyleVar(2);

		ImGui::PopStyleVar(2);

		// Dockspace
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		ImGui::DockSpace(ImGui::GetID("MyDockspace"));
		style.WindowMinSize.x = minWinSizeX;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		if (ImGui::Begin("Viewport"))
		{
			ImVec2 viewportSize = ImGui::GetContentRegionAvail();

			if (m_MeshRenderer && (viewportSize.x != m_LastViewportSize.x || viewportSize.y != m_LastViewportSize.y))
			{
				if (viewportSize.x > 0 && viewportSize.y > 0)
				{
					m_LastViewportSize = viewportSize;
				}
			}

			if (m_MeshRenderer && viewportSize.x > 0 && viewportSize.y > 0)
			{
				Ref<Image2D> renderedImage = m_MeshRenderer->GetImage(0);
				if (renderedImage)
				{
					ImTextureID textureID = m_MeshRenderer->GetTextureImGuiID(renderedImage);
					if (textureID)
					{
						ImGui::Image(textureID, viewportSize);
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
	}

	bool EditorLayer::OnEvent(Event& e)
	{ return false; }

}
#include "EditorLayer.hpp"
#include "Zenith/Core/ApplicationContext.hpp"

#include "Zenith/Debug/Profiler.hpp"

#include "Zenith/Project/Project.hpp"
#include "Zenith/Project/ProjectSerializer.hpp"
#include "Zenith/Utilities/FileSystem.hpp"
#include "Zenith/Utilities/StringUtils.hpp"

#include "Zenith/Asset/MeshImporter.hpp"

#include <imgui/imgui_internal.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <future>

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
	}

	void EditorLayer::SetApplicationContext(std::shared_ptr<ApplicationContext> context)
	{
		m_ApplicationContext = context;
	}

	void EditorLayer::OnDetach()
	{
		CloseProject(true);
		Project::SetActive(nullptr);
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

	void EditorLayer::RenderMeshTestUI()
	{
		if (ImGui::Begin("Mesh Loading Test"))
		{
			ImGui::SeparatorText("Mesh Loading Test");
			if (ImGui::Button("Load Cube.glb", ImVec2(-1, 30)))
			{
				TestLoadMesh();
			}
			ImGui::Separator();

			if (m_MeshLoadSuccess)
			{
				ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Mesh Loaded Successfully");
			}
			else if (!m_MeshTestLog.empty())
			{
				ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Load Failed");
			}
			else
			{
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No test performed yet");
			}

			if (m_MeshLoadSuccess && m_TestMeshSource)
			{
				ImGui::SeparatorText("Mesh Statistics");

				ImGui::Text("Vertices: %u", m_LoadedVertexCount);
				ImGui::Text("Indices: %u", m_LoadedIndexCount);
				ImGui::Text("Submeshes: %u", m_LoadedSubmeshCount);
				ImGui::Text("Materials: %zu", m_TestMeshSource->GetMaterials().size());

				const auto& bb = m_TestMeshSource->GetBoundingBox();
				ImGui::Text("Bounding Box:");
				ImGui::Text("  Min: (%.2f, %.2f, %.2f)", bb.Min.x, bb.Min.y, bb.Min.z);
				ImGui::Text("  Max: (%.2f, %.2f, %.2f)", bb.Max.x, bb.Max.y, bb.Max.z);

				if (ImGui::TreeNode("Submesh Details"))
				{
					const auto& submeshes = m_TestMeshSource->GetSubmeshes();
					for (size_t i = 0; i < submeshes.size(); i++)
					{
						const auto& submesh = submeshes[i];
						if (ImGui::TreeNode(("Submesh " + std::to_string(i)).c_str()))
						{
							ImGui::Text("Name: %s", submesh.MeshName.c_str());
							ImGui::Text("Vertices: %u (Base: %u)", submesh.VertexCount, submesh.BaseVertex);
							ImGui::Text("Indices: %u (Base: %u)", submesh.IndexCount, submesh.BaseIndex);
							ImGui::Text("Material Index: %u", submesh.MaterialIndex);

							const auto& subBB = submesh.BoundingBox;
							ImGui::Text("Bounding Box:");
							ImGui::Text("  Min: (%.2f, %.2f, %.2f)", subBB.Min.x, subBB.Min.y, subBB.Min.z);
							ImGui::Text("  Max: (%.2f, %.2f, %.2f)", subBB.Max.x, subBB.Max.y, subBB.Max.z);

							ImGui::TreePop();
						}
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("GPU Buffers"))
				{
					auto vertexBuffer = m_TestMeshSource->GetVertexBuffer();
					if (vertexBuffer)
						ImGui::Text("Vertex Buffer: %u bytes", vertexBuffer->GetSize());
					else
						ImGui::Text("Vertex Buffer: Not created");

					auto indexBuffer = m_TestMeshSource->GetIndexBuffer();
					if (indexBuffer)
						ImGui::Text("Index Buffer: %u indices", indexBuffer->GetCount());
					else
						ImGui::Text("Index Buffer: Not created");

					ImGui::TreePop();
				}
			}

			if (!m_MeshTestLog.empty())
			{
				ImGui::SeparatorText("Log Output");
				ImGui::TextWrapped("%s", m_MeshTestLog.c_str());
			}
		}
		ImGui::End();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		ZN_PROFILE_FUNC();
	}

	void EditorLayer::OnImGuiRender()
	{
		ZN_PROFILE_FUNC();

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
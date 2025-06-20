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
			ZN_CORE_VERIFY(false, "No project provided!");

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
			ZN_CORE_WARN("EditorLayer: No application context available for window title update");
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
			ZN_CORE_VERIFY(stream.is_open());
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
			ZN_CORE_ERROR("Tried to open a project that doesn't exist. Project path: {0}", filepath);
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
			ZN_CORE_VERIFY(false); // TODO

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
	}

	bool EditorLayer::OnEvent(Event& e)
	{ return false; }

}
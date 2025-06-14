#include "EditorLayer.hpp"

#include "Zenith/Debug/Profiler.hpp"

#include "Zenith/Utilities/FileSystem.hpp"

#include <imgui_internal.h>

#include <filesystem>

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

			Application& app = Application::Get();
			bool vsync = app.GetWindow().IsVSync();
			if (ImGui::Checkbox("VSync", &vsync))
				app.GetWindow().SetVSync(vsync);

			ImGui::SeparatorText("File Operations");

			if (ImGui::Button("Open File Dialog", ImVec2(-1, 0)))
			{
				std::initializer_list<FileSystem::FileDialogFilterItem> filters = {
					{"Text Files", "txt"},
					{"Images", "png,jpg,jpeg,bmp,tga"},
				};

				std::filesystem::path filePath = Zenith::FileSystem::OpenFileDialog(filters);

				if (!filePath.empty())
					ZN_INFO("Selected file: {0}", filePath.string());
				else
					ZN_INFO("No file selected or dialog cancelled.");
			}
		}
		ImGui::End();

		if (ImGui::Begin("Controller Test"))
		{
			static int controllerID_UI = 1;
			static int buttonID = 0;
			static int axisID = 0;

			ImGui::SliderInt("Controller ID", &controllerID_UI, 1, 4);
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

	void EditorLayer::OnEvent(Event& e)
	{}


}
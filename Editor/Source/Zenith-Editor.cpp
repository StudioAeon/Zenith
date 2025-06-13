#include "EditorLayer.hpp"
#include "Zenith/Utilities/FileSystem.hpp"

#include "Zenith/EntryPoint.hpp"

class ZenithEditorApplication : public Zenith::Application
{
public:
	ZenithEditorApplication(const Zenith::ApplicationSpecification& specification)
		: Application(specification)
	{
		auto editorLayer = std::make_shared<Zenith::EditorLayer>();
		editorLayer->SetEnabled(true);

		// Persistent Storage
		{
			m_PersistentStoragePath = Zenith::FileSystem::GetPersistentStoragePath() / "Zenith-Editor";

			if (!Zenith::FileSystem::Exists(m_PersistentStoragePath))
				Zenith::FileSystem::CreateDirectory(m_PersistentStoragePath);
		}

		// Update the ZENITH_DIR config entry every time we launch
		{
			auto workingDirectory = Zenith::FileSystem::GetWorkingDirectory();

			if (workingDirectory.stem().string() == "Zenith-Editor")
				workingDirectory = workingDirectory.parent_path();

			Zenith::FileSystem::SetConfigValue("ZENITH_DIR", workingDirectory.string());
		}

		PushLayer(editorLayer);
	}

private:
	std::filesystem::path m_PersistentStoragePath;
};

Zenith::Application* Zenith::CreateApplication(int argv, char** argc)
{
	ApplicationSpecification specification;
	specification.Name = "Zenith-Editor";
	specification.WindowWidth = 1920;
	specification.WindowHeight = 1080;
	specification.StartMaximized = true;
	specification.VSync = true;

	return new ZenithEditorApplication(specification);
}
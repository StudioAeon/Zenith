#include "EditorLayer.hpp"
#include "Zenith/Utilities/CommandLineParser.hpp"
#include "Zenith/Utilities/FileSystem.hpp"
#include "Zenith/Core/ApplicationContext.hpp"

#include "Zenith/EntryPoint.hpp"

class ZenithEditorApplication : public Zenith::Application
{
public:
	ZenithEditorApplication(const Zenith::ApplicationSpecification& specification, std::string_view projectPath)
		: Application(specification), m_ProjectPath(projectPath), m_UserPreferences(Zenith::Ref<Zenith::UserPreferences>::Create())
	{
		if (projectPath.empty())
			m_ProjectPath = "ProjectApex/Apex.zproj";
	}

	virtual void OnInit() override
	{
		// Persistent Storage
		{
			m_PersistentStoragePath = Zenith::FileSystem::GetPersistentStoragePath() / "Zenith-Editor";

			if (!Zenith::FileSystem::Exists(m_PersistentStoragePath))
				Zenith::FileSystem::CreateDirectory(m_PersistentStoragePath);
		}

		// User Preferences
		{
			Zenith::UserPreferencesSerializer serializer(m_UserPreferences);
			if (!Zenith::FileSystem::Exists(m_PersistentStoragePath / "UserPreferences.json"))
				serializer.Serialize(m_PersistentStoragePath / "UserPreferences.json");
			else
				serializer.Deserialize(m_PersistentStoragePath / "UserPreferences.json");

			if (!m_ProjectPath.empty())
				m_UserPreferences->StartupProject = m_ProjectPath;
			else if (!m_UserPreferences->StartupProject.empty())
				m_ProjectPath = m_UserPreferences->StartupProject;
		}

		// Update the ZENITH_DIR config entry every time we launch
		{
			auto workingDirectory = Zenith::FileSystem::GetWorkingDirectory();

			if (workingDirectory.stem().string() == "Zenith-Editor")
				workingDirectory = workingDirectory.parent_path();

			Zenith::FileSystem::SetConfigValue("ZENITH_DIR", workingDirectory.string());
		}

		auto editorLayer = std::make_shared<Zenith::EditorLayer>(m_UserPreferences);
		editorLayer->SetEnabled(true);

		auto applicationContext = GetApplicationContext();
		editorLayer->SetApplicationContext(applicationContext);

		PushLayer(editorLayer);

		m_ApplicationContext = std::move(applicationContext);
	}

private:
	std::string m_ProjectPath;
	std::filesystem::path m_PersistentStoragePath;
	Zenith::Ref<Zenith::UserPreferences> m_UserPreferences;

	std::shared_ptr<Zenith::ApplicationContext> m_ApplicationContext;
};

Zenith::Application* Zenith::CreateApplication(int argc, char** argv)
{
	Zenith::CommandLineParser cli(argc, argv);

	auto raw = cli.GetRawArgs();
	if(raw.size() > 1) {
		ZN_CORE_WARN("More than one project path specified, using `{}'", raw[0]);
	}

	auto cd = cli.GetOptionValue("C");
	if(!cd.empty()) {
		Zenith::FileSystem::SetWorkingDirectory(cd);
	}

	std::string_view projectPath;
	if(!raw.empty()) projectPath = raw[0];

	ApplicationSpecification specification;
	specification.Name = "Zenith-Editor";
	specification.WindowWidth = 1920;
	specification.WindowHeight = 1080;
	specification.StartMaximized = true;
	specification.VSync = true;

	return new ZenithEditorApplication(specification, projectPath);
}
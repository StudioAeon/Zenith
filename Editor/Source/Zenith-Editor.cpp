#include "EditorLayer.hpp"

#include "Zenith/EntryPoint.hpp"

class ZenithEditorApplication : public Zenith::Application
{
public:
	ZenithEditorApplication(const Zenith::ApplicationSpecification& specification)
		: Application(specification)
	{
		auto editorLayer = std::make_shared<Zenith::EditorLayer>();
		editorLayer->SetEnabled(true);
		PushLayer(editorLayer);
	}
};

Zenith::Application* Zenith::CreateApplication(int argv, char** argc)
{
	Zenith::ApplicationSpecification specification;
	specification.Name = "Zenith-Editor";
	specification.WindowWidth = 1920;
	specification.WindowHeight = 1080;
	specification.StartMaximized = true;
	specification.VSync = true;

	return new ZenithEditorApplication(specification);
}
#include "Zenith.hpp"

#include "Zenith/EntryPoint.hpp"

class ZenithEditorApplication : public Zenith::Application
{
public:
	ZenithEditorApplication()
	{}
};

Zenith::Application* Zenith::CreateApplication(int argv, char** argc)
{
	return new ZenithEditorApplication();
}
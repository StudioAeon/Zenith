#pragma once

#include "Zenith/Core/Application.hpp"
#include "Zenith/Core/Assert.hpp"

extern Zenith::Application* Zenith::CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

namespace Zenith {

	int Main(int argc, char** argv)
	{
		while (g_ApplicationRunning)
		{
			InitializeCore();
			Application* app = CreateApplication(argc, argv);
			ZN_CORE_ASSERT(app, "Client Application is null!");
			app->Run();
			delete app;
			ShutdownCore();
		}
		return 0;
	}

}

#if defined(ZN_DIST) && defined(ZN_PLATFORM_WINDOWS)

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	return Zenith::Main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
	return Zenith::Main(argc, argv);
}

#endif
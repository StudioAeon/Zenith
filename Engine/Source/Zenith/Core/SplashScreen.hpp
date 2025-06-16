#pragma once

#include "Zenith/Core/Base.hpp"
#include <SDL3/SDL.h>
#include <string>
#include <filesystem>

namespace Zenith {

	class SplashScreen
	{
	public:
		struct Config
		{
			std::filesystem::path ImagePath = "Resources/Editor/Zenith_Splash.png";
			uint32_t WindowWidth = 800;
			uint32_t WindowHeight = 600;
			float DisplayTime = 1.5f;
			bool AllowSkip = true;
			SDL_Color BackgroundColor = { 0, 0, 0, 255 }; // Fallback if image fails
		};

		explicit SplashScreen();
		explicit SplashScreen(const Config& config);
		~SplashScreen();

		bool Initialize();
		void Show();
		bool ShouldClose() const;

	private:
		bool LoadSplashImage();
		void Render();
		void RenderFullscreenQuad();

		SDL_Window* m_Window = nullptr;
		SDL_Renderer* m_Renderer = nullptr;
		SDL_Texture* m_SplashTexture = nullptr;
		Config m_Config;

		uint32_t m_StartTime = 0;
		bool m_Initialized = false;
		bool m_ImageLoaded = false;

		int m_ImageWidth = 0;
		int m_ImageHeight = 0;
	};

}
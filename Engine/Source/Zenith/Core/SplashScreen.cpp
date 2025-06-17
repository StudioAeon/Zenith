#include "znpch.hpp"
#include "SplashScreen.hpp"

#include <stb/stb_image.h>

namespace Zenith {

	SplashScreen::SplashScreen() : SplashScreen({}) {}

	SplashScreen::SplashScreen(const Config& config) : m_Config(config) {}

	SplashScreen::~SplashScreen()
	{
		if (m_SplashTexture) SDL_DestroyTexture(m_SplashTexture);
		if (m_Renderer) SDL_DestroyRenderer(m_Renderer);
		if (m_Window) SDL_DestroyWindow(m_Window);
	}

	bool SplashScreen::Initialize()
	{
		m_Window = SDL_CreateWindow(
			"Zenith Engine",
			m_Config.WindowWidth, m_Config.WindowHeight,
			SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP
		);

		if (!m_Window)
		{
			ZN_CORE_ERROR("Failed to create splash window: {}", SDL_GetError());
			return false;
		}

		SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

		m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
		if (!m_Renderer)
		{
			m_Renderer = SDL_CreateRenderer(m_Window, "software");
		}

		if (!m_Renderer)
		{
			ZN_CORE_ERROR("Failed to create splash renderer: {}", SDL_GetError());
			return false;
		}

		m_ImageLoaded = LoadSplashImage();
		if (!m_ImageLoaded)
		{
			ZN_CORE_WARN("Failed to load splash image, using fallback");
		}

		m_Initialized = true;
		return true;
	}

	bool SplashScreen::LoadSplashImage()
	{
		if (!std::filesystem::exists(m_Config.ImagePath))
		{
			ZN_CORE_WARN("Splash image not found: {}", m_Config.ImagePath.string());
			return false;
		}

		int channels;
		unsigned char* imageData = stbi_load(
			m_Config.ImagePath.string().c_str(),
			&m_ImageWidth, &m_ImageHeight, &channels, 4
		);

		if (!imageData)
		{
			ZN_CORE_ERROR("Failed to load splash image: {}", stbi_failure_reason());
			return false;
		}

		m_SplashTexture = SDL_CreateTexture(
			m_Renderer,
			SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STATIC,
			m_ImageWidth, m_ImageHeight
		);

		if (!m_SplashTexture)
		{
			ZN_CORE_ERROR("Failed to create splash texture: {}", SDL_GetError());
			stbi_image_free(imageData);
			return false;
		}

		SDL_UpdateTexture(m_SplashTexture, nullptr, imageData, m_ImageWidth * 4);
		SDL_SetTextureBlendMode(m_SplashTexture, SDL_BLENDMODE_BLEND);

		stbi_image_free(imageData);
		return true;
	}

	void SplashScreen::Show()
	{
		if (!m_Initialized) return;

		m_StartTime = SDL_GetTicks();

		Render();

		while (!ShouldClose())
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (m_Config.AllowSkip)
				{
					if (event.type == SDL_EVENT_KEY_DOWN ||
						event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
					{
						return;
					}
				}
			}

			SDL_Delay(16);
		}
	}

	bool SplashScreen::ShouldClose() const
	{
		if (!m_Initialized) return true;

		uint32_t elapsed = SDL_GetTicks() - m_StartTime;
		return elapsed >= (m_Config.DisplayTime * 1000.0f);
	}

	void SplashScreen::Render()
	{
		SDL_SetRenderDrawColor(m_Renderer,
			m_Config.BackgroundColor.r, m_Config.BackgroundColor.g,
			m_Config.BackgroundColor.b, m_Config.BackgroundColor.a);
		SDL_RenderClear(m_Renderer);

		if (m_ImageLoaded && m_SplashTexture)
		{
			RenderFullscreenQuad();
		}

		SDL_RenderPresent(m_Renderer);
	}

	void SplashScreen::RenderFullscreenQuad()
	{
		float windowAspect = static_cast<float>(m_Config.WindowWidth) / m_Config.WindowHeight;
		float imageAspect = static_cast<float>(m_ImageWidth) / m_ImageHeight;

		SDL_FRect destRect;

		if (imageAspect > windowAspect)
		{
			destRect.w = static_cast<float>(m_Config.WindowWidth);
			destRect.h = destRect.w / imageAspect;
			destRect.x = 0;
			destRect.y = (m_Config.WindowHeight - destRect.h) * 0.5f;
		}
		else
		{
			destRect.h = static_cast<float>(m_Config.WindowHeight);
			destRect.w = destRect.h * imageAspect;
			destRect.x = (m_Config.WindowWidth - destRect.w) * 0.5f;
			destRect.y = 0;
		}

		SDL_RenderTexture(m_Renderer, m_SplashTexture, nullptr, &destRect);
	}

}
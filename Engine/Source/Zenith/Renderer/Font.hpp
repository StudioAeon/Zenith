#pragma once

#include "Zenith/Core/Buffer.hpp"
#include <glm/glm.hpp>

#include <stb/stb_truetype.h>

#include <filesystem>
#include <string>
#include <memory>
#include <unordered_map>
#include <array>

namespace Zenith {

	struct FontAtlasData
	{
		int Width = 0;
		int Height = 0;
		Buffer Bitmap;

		static constexpr int FirstChar = 32;
		static constexpr int LastChar = 126;
		static constexpr int NumChars = LastChar - FirstChar + 1;

		std::array<stbtt_packedchar, NumChars> PackedChars;

		float FontSize = 0.0f;
		float Scale = 0.0f;
		int Ascent = 0;
		int Descent = 0;
		int LineGap = 0;

		uint32_t TextureID = 0;
	};

	class Font : public std::enable_shared_from_this<Font>
	{
	public:
		Font(const std::filesystem::path& filepath);
		Font(const std::string& name, Buffer buffer);
		virtual ~Font();

		Font(const Font&) = delete;
		Font& operator=(const Font&) = delete;

		const FontAtlasData* GetFontAtlas() const { return m_AtlasData.get(); }
		uint32_t GetAtlasTextureID() const { return m_AtlasData ? m_AtlasData->TextureID : 0; }

		const std::string& GetName() const { return m_Name; }
		const std::filesystem::path& GetFilePath() const { return m_FilePath; }
		float GetFontSize() const { return m_AtlasData ? m_AtlasData->FontSize : 0.0f; }

		glm::vec2 MeasureText(const std::string& text, float scale = 1.0f) const;
		float GetLineHeight(float scale = 1.0f) const;

		const stbtt_packedchar* GetPackedChar(char c) const;
		bool IsCharacterSupported(char c) const;

		static void Init();
		static void Shutdown();

		static std::shared_ptr<Font> GetDefaultFont();
		static std::shared_ptr<Font> GetDefaultMonoSpacedFont();

		static std::shared_ptr<Font> LoadFont(const std::filesystem::path& filepath, float fontSize = 48.0f);
		static std::shared_ptr<Font> LoadFont(const std::string& name, const std::filesystem::path& filepath, float fontSize = 48.0f);
		static std::shared_ptr<Font> GetFont(const std::string& name);

	private:
		void CreateAtlas(Buffer buffer, float fontSize = 48.0f);
		bool LoadFromFile(const std::filesystem::path& filepath, float fontSize = 48.0f);
		static void LoadDefaultFonts();

	private:
		std::string m_Name;
		std::filesystem::path m_FilePath;
		Buffer m_FontBuffer;
		std::unique_ptr<stbtt_fontinfo> m_FontInfo;
		std::unique_ptr<FontAtlasData> m_AtlasData;

		inline static std::unordered_map<std::string, std::shared_ptr<Font>> s_FontRegistry;
		inline static std::shared_ptr<Font> s_DefaultFont;
		inline static std::shared_ptr<Font> s_DefaultMonoSpacedFont;
		inline static bool s_Initialized = false;
	};

}

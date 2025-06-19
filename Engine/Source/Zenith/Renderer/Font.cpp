#include "znpch.hpp"
#include "Font.hpp"

#include "Zenith/Asset/AssetManager.hpp"

#include <fstream>

namespace Zenith {

	Font::Font(const std::filesystem::path& filepath)
		: m_FilePath(filepath), m_Name(filepath.stem().string())
	{
		m_FontInfo = std::make_unique<stbtt_fontinfo>();

		if (!LoadFromFile(filepath))
		{
			ZN_CORE_ERROR("Failed to load font from file: {}", filepath.string());
		}
	}

	Font::Font(const std::string& name, Buffer buffer)
		: m_Name(name)
	{
		m_FontInfo = std::make_unique<stbtt_fontinfo>();

		if (buffer)
		{
			CreateAtlas(buffer);
		}
		else
		{
			ZN_CORE_ERROR("Invalid buffer provided for font: {}", name);
		}
	}

	Font::~Font() {}

	bool Font::LoadFromFile(const std::filesystem::path& filepath, float fontSize)
	{
		std::ifstream file(filepath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			ZN_CORE_ERROR("Failed to open font file: {}", filepath.string());
			return false;
		}

		std::streamsize fileSize = file.tellg();
		if (fileSize <= 0)
		{
			ZN_CORE_ERROR("Font file is empty: {}", filepath.string());
			return false;
		}

		file.seekg(0, std::ios::beg);

		m_FontBuffer.Allocate(static_cast<uint64_t>(fileSize));
		if (!file.read(static_cast<char*>(m_FontBuffer.Data), fileSize))
		{
			ZN_CORE_ERROR("Failed to read font file: {}", filepath.string());
			m_FontBuffer.Release();
			return false;
		}

		CreateAtlas(m_FontBuffer, fontSize);

		return m_AtlasData != nullptr;
	}

	void Font::CreateAtlas(Buffer buffer, float fontSize)
	{
		if (!buffer)
		{
			ZN_CORE_ERROR("Invalid font buffer for: {}", m_Name);
			return;
		}

		if (!stbtt_InitFont(m_FontInfo.get(), static_cast<const unsigned char*>(buffer.Data), 0))
		{
			ZN_CORE_ERROR("Failed to initialize font: {}", m_Name);
			return;
		}

		m_AtlasData = std::make_unique<FontAtlasData>();
		m_AtlasData->FontSize = fontSize;
		m_AtlasData->Scale = stbtt_ScaleForPixelHeight(m_FontInfo.get(), fontSize);
		stbtt_GetFontVMetrics(m_FontInfo.get(), &m_AtlasData->Ascent, &m_AtlasData->Descent, &m_AtlasData->LineGap);

		m_AtlasData->Width = 512;
		m_AtlasData->Height = 512;
		m_AtlasData->Bitmap.Allocate(m_AtlasData->Width * m_AtlasData->Height);

		stbtt_pack_context packContext;
		if (!stbtt_PackBegin(&packContext, static_cast<unsigned char*>(m_AtlasData->Bitmap.Data),
			m_AtlasData->Width, m_AtlasData->Height, 0, 1, nullptr))
		{
			ZN_CORE_ERROR("Failed to initialize font packing for: {}", m_Name);
			m_AtlasData.reset();
			return;
		}

		stbtt_PackSetOversampling(&packContext, 1, 1);

		if (!stbtt_PackFontRange(&packContext, static_cast<const unsigned char*>(buffer.Data), 0, fontSize,
			FontAtlasData::FirstChar, FontAtlasData::NumChars,
			m_AtlasData->PackedChars.data()))
		{
			ZN_CORE_ERROR("Failed to pack font characters for: {}", m_Name);
			stbtt_PackEnd(&packContext);
			m_AtlasData.reset();
			return;
		}

		stbtt_PackEnd(&packContext);

		if (&buffer != &m_FontBuffer)
		{
			m_FontBuffer = BufferSafe::Copy(buffer.Data, buffer.Size);
		}
	}

	glm::vec2 Font::MeasureText(const std::string& text, float scale) const
	{
		if (!m_AtlasData) return glm::vec2(0.0f);

		float width = 0.0f;
		float maxLineWidth = 0.0f;
		int lineCount = 1;

		for (char c : text)
		{
			if (c == '\n')
			{
				maxLineWidth = std::max(maxLineWidth, width);
				width = 0.0f;
				lineCount++;
				continue;
			}

			const auto* packedChar = GetPackedChar(c);
			if (packedChar)
			{
				width += packedChar->xadvance * scale;
			}
		}

		maxLineWidth = std::max(maxLineWidth, width);
		float height = GetLineHeight(scale) * lineCount;

		return glm::vec2(maxLineWidth, height);
	}

	float Font::GetLineHeight(float scale) const
	{
		if (!m_AtlasData) return 0.0f;
		return (m_AtlasData->Ascent - m_AtlasData->Descent + m_AtlasData->LineGap) * m_AtlasData->Scale * scale;
	}

	const stbtt_packedchar* Font::GetPackedChar(char c) const
	{
		if (!IsCharacterSupported(c)) return nullptr;

		int index = static_cast<int>(c) - FontAtlasData::FirstChar;
		return &m_AtlasData->PackedChars[index];
	}

	bool Font::IsCharacterSupported(char c) const
	{
		return c >= FontAtlasData::FirstChar && c <= FontAtlasData::LastChar;
	}

	void Font::Init()
	{
		if (s_Initialized)
		{
			ZN_CORE_WARN_TAG("Font", "Font system already initialized");
			return;
		}

		s_FontNameRegistry.clear();
		s_DefaultFontHandle = AssetHandle();
		s_DefaultMonoSpacedFontHandle = AssetHandle();

		s_Initialized = true;

		LoadDefaultFonts();
	}

	void Font::Shutdown()
	{
		if (!s_Initialized)
		{
			ZN_CORE_WARN_TAG("Font", "Font system not initialized");
			return;
		}

		s_FontNameRegistry.clear();
		s_DefaultFontHandle = AssetHandle();
		s_DefaultMonoSpacedFontHandle = AssetHandle();
		s_Initialized = false;
	}

	Ref<Font> Font::GetDefaultFont()
	{
		if (s_DefaultFontHandle == 0)
			return nullptr;

		if (Project::GetActive())
		{
			if (auto font = AssetManager::GetAsset<Font>(s_DefaultFontHandle))
				return font;
		}

		auto it = s_InitializationFonts.find(s_DefaultFontHandle);
		if (it != s_InitializationFonts.end())
			return it->second;

		return nullptr;
	}

	Ref<Font> Font::GetDefaultMonoSpacedFont()
	{
		if (s_DefaultMonoSpacedFontHandle == 0)
			return nullptr;

		return AssetManager::GetAsset<Font>(s_DefaultMonoSpacedFontHandle);
	}

	AssetHandle Font::CreateFont(const std::filesystem::path& filepath, float fontSize)
	{
		if (!s_Initialized)
		{
			ZN_CORE_ERROR_TAG("Font", "Font system not initialized");
			return AssetHandle::null();
		}

		std::string name = filepath.stem().string();
		return CreateFont(name, filepath, fontSize);
	}

	AssetHandle Font::CreateFont(const std::string& name, const std::filesystem::path& filepath, float fontSize)
	{
		if (!s_Initialized)
		{
			ZN_CORE_ERROR_TAG("Font", "Font system not initialized");
			return AssetHandle::null();
		}

		if (auto existingHandle = FindFontByName(name); existingHandle != 0)
		{
			ZN_CORE_WARN_TAG("Font", "Font '{}' already loaded", name);
			return existingHandle;
		}

		auto font = Ref<Font>::Create(filepath);
		if (font && font->GetFontAtlas())
		{
			font->m_Name = name;

			// Only register with AssetManager if Project system is available
			if (Project::GetActive())
			{
				AssetHandle handle = AssetManager::AddMemoryOnlyAsset(font);
				s_FontNameRegistry[name] = handle;
				return handle;
			}
			else
			{
				// During initialization - create handle directly without AssetManager
				font->Handle = AssetHandle();
				s_FontNameRegistry[name] = font->Handle;

				// Store font temporarily until AssetManager becomes available
				s_InitializationFonts[font->Handle] = font;

				return font->Handle;
			}
		}

		ZN_CORE_ERROR_TAG("Font", "Failed to load font '{}' from '{}'", name, filepath.string());
		return AssetHandle::null();
	}

	Ref<Font> Font::GetFont(const std::string& name)
	{
		if (!s_Initialized)
		{
			ZN_CORE_ERROR_TAG("Font", "Font system not initialized");
			return nullptr;
		}

		if (auto handle = FindFontByName(name); handle != 0)
			return AssetManager::GetAsset<Font>(handle);

		ZN_CORE_WARN_TAG("Font", "Font '{}' not found", name);
		return nullptr;
	}

	AssetHandle Font::FindFontByName(const std::string& name)
	{
		auto it = s_FontNameRegistry.find(name);
		if (it != s_FontNameRegistry.end())
			return it->second;

		// Only search through AssetManager if Project system is available
		if (Project::GetActive())
		{
			auto fontHandles = AssetManager::GetAllAssetsWithType<Font>();
			for (AssetHandle handle : fontHandles)
			{
				if (auto font = AssetManager::GetAsset<Font>(handle))
				{
					if (font->GetName() == name)
					{
						s_FontNameRegistry[name] = handle;
						return handle;
					}
				}
			}
		}

		return AssetHandle::null();
	}

	void Font::LoadDefaultFonts()
	{
#ifdef ZN_PLATFORM_WINDOWS
		std::vector<std::filesystem::path> defaultFonts = {
			"C:/Windows/Fonts/arial.ttf",
			"C:/Windows/Fonts/calibri.ttf",
			"C:/Windows/Fonts/segoeui.ttf"
		};

		std::vector<std::filesystem::path> monoFonts = {
			"C:/Windows/Fonts/consola.ttf",
			"C:/Windows/Fonts/courbd.ttf",
			"C:/Windows/Fonts/cour.ttf"
		};
#elif defined(ZN_PLATFORM_UNIX)
		std::vector<std::filesystem::path> defaultFonts = {
			"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
			"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
			"/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"
		};

		std::vector<std::filesystem::path> monoFonts = {
			"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
			"/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
			"/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf"
		};
#endif

		// Try to load default font
		for (const auto& fontPath : defaultFonts)
		{
			if (std::filesystem::exists(fontPath))
			{
				s_DefaultFontHandle = CreateFont("default", fontPath);
				if (s_DefaultFontHandle != 0)
					break;
			}
		}

		// Try to load default monospace font
		for (const auto& fontPath : monoFonts)
		{
			if (std::filesystem::exists(fontPath))
			{
				s_DefaultMonoSpacedFontHandle = CreateFont("monospace", fontPath);
				if (s_DefaultMonoSpacedFontHandle != 0)
					break;
			}
		}

		if (s_DefaultFontHandle == 0)
			ZN_CORE_WARN_TAG("Font", "Failed to load any default font");
		if (s_DefaultMonoSpacedFontHandle == 0)
			ZN_CORE_WARN_TAG("Font", "Failed to load any default monospace font");
	}

}
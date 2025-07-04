#pragma once

#include "Zenith/Renderer/Texture.hpp"
#include "Zenith/Core/Buffer.hpp"

#include <filesystem>

namespace Zenith {

	struct TextureData
	{
		Buffer ImageData;
		uint32_t Width = 0;
		uint32_t Height = 0;
		ImageFormat Format = ImageFormat::RGBA;

		bool IsValid() const { return ImageData.Data != nullptr && Width > 0 && Height > 0; }
	};

	class TextureImporter
	{
	public:
		static TextureData LoadTextureData(const std::filesystem::path& path);
		static TextureData LoadTextureData(Buffer buffer);
		static TextureData LoadTextureData(const std::filesystem::path& path, ImageFormat preferredFormat);
		static TextureData LoadTextureData(Buffer buffer, ImageFormat preferredFormat);

		static Ref<Texture2D> CreateTexture(const TextureData& textureData, const std::string& debugName = "");

		//Keep for backwards compatibility
		static Buffer ToBufferFromFile(const std::filesystem::path& path, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);
		static Buffer ToBufferFromMemory(Buffer buffer, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);

	private:
		const std::filesystem::path m_Path;
	};

}
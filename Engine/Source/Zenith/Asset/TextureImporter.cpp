#include "znpch.hpp"
#include "TextureImporter.hpp"

#include "Zenith/Renderer/Texture.hpp"
#include "Zenith/Utilities/FileSystem.hpp"

#include <stb/stb_image.h>

namespace Zenith {

	TextureData TextureImporter::LoadTextureData(const std::filesystem::path& path)
	{
		TextureData result;

		std::string pathStr = path.string();
		int width, height, channels;
		void* tmp;
		size_t size = 0;

		FileStatus fileStatus = FileSystem::TryOpenFileAndWait(path, 100);

		if (stbi_is_hdr(pathStr.c_str()))
		{
			tmp = stbi_loadf(pathStr.c_str(), &width, &height, &channels, 4);
			if (tmp)
			{
				size = width * height * 4 * sizeof(float);
				result.Format = ImageFormat::RGBA32F;
			}
		}
		else
		{
			stbi_set_flip_vertically_on_load(1);
			tmp = stbi_load(pathStr.c_str(), &width, &height, &channels, 4);
			if (tmp)
			{
				size = width * height * 4;
				result.Format = ImageFormat::RGBA;
			}
		}

		if (!tmp)
		{
			ZN_CORE_ERROR_TAG("TextureImporter", "Failed to load texture: {}", pathStr);
			return result;
		}

		result.Width = static_cast<uint32_t>(width);
		result.Height = static_cast<uint32_t>(height);
		result.ImageData = Buffer::Copy(tmp, size);

		stbi_image_free(tmp);

		return result;
	}

	TextureData TextureImporter::LoadTextureData(Buffer buffer)
	{
		TextureData result;

		int width, height, channels;
		void* tmp;
		size_t size;

		// Check if HDR texture from memory
		if (stbi_is_hdr_from_memory((const stbi_uc*)buffer.Data, (int)buffer.Size))
		{
			tmp = stbi_loadf_from_memory((const stbi_uc*)buffer.Data, (int)buffer.Size, &width, &height, &channels, 4);
			size = width * height * 4 * sizeof(float);
			result.Format = ImageFormat::RGBA32F;
		}
		else
		{
			stbi_set_flip_vertically_on_load(1);
			tmp = stbi_load_from_memory((const stbi_uc*)buffer.Data, (int)buffer.Size, &width, &height, &channels, 4);
			size = width * height * 4;
			result.Format = ImageFormat::RGBA;
		}

		if (!tmp)
		{
			ZN_CORE_ERROR_TAG("TextureImporter", "Failed to load texture from memory");
			return result;
		}

		result.Width = static_cast<uint32_t>(width);
		result.Height = static_cast<uint32_t>(height);
		result.ImageData = Buffer::Copy(tmp, size);

		stbi_image_free(tmp);

		return result;
	}

	TextureData TextureImporter::LoadTextureData(const std::filesystem::path& path, ImageFormat preferredFormat)
	{
		TextureData result = LoadTextureData(path);

		if (result.IsValid() && result.Format == ImageFormat::RGBA)
		{
			if (preferredFormat == ImageFormat::SRGBA || preferredFormat == ImageFormat::SRGB)
			{
				result.Format = ImageFormat::SRGBA;
			}
		}

		return result;
	}

	TextureData TextureImporter::LoadTextureData(Buffer buffer, ImageFormat preferredFormat)
	{
		TextureData result = LoadTextureData(buffer);

		if (result.IsValid() && result.Format == ImageFormat::RGBA)
		{
			if (preferredFormat == ImageFormat::SRGBA || preferredFormat == ImageFormat::SRGB)
			{
				result.Format = ImageFormat::SRGBA;
			}
		}

		return result;
	}

	Ref<Texture2D> TextureImporter::CreateTexture(const TextureData& textureData, const std::string& debugName)
	{
		if (!textureData.IsValid())
		{
			ZN_CORE_ERROR_TAG("TextureImporter", "Invalid texture data provided");
			return nullptr;
		}

		TextureSpecification spec;
		spec.Width = textureData.Width;
		spec.Height = textureData.Height;
		spec.Format = textureData.Format;
		spec.GenerateMips = true;
		spec.DebugName = debugName;

		// Use existing static method for now - will be replaced with ResourceManager later
		return Texture2D::Create(spec, textureData.ImageData);
	}

	Buffer TextureImporter::ToBufferFromFile(const std::filesystem::path& path, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
		bool isSRGB = (outFormat == ImageFormat::SRGB) || (outFormat == ImageFormat::SRGBA);

		TextureData data = LoadTextureData(path, isSRGB ? ImageFormat::SRGBA : ImageFormat::RGBA);
		outFormat = data.Format;
		outWidth = data.Width;
		outHeight = data.Height;
		return data.ImageData;
	}

	Buffer TextureImporter::ToBufferFromMemory(Buffer buffer, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
		bool isSRGB = (outFormat == ImageFormat::SRGB) || (outFormat == ImageFormat::SRGBA);

		TextureData data = LoadTextureData(buffer, isSRGB ? ImageFormat::SRGBA : ImageFormat::RGBA);
		outFormat = data.Format;
		outWidth = data.Width;
		outHeight = data.Height;
		return data.ImageData;
	}

}
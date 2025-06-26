#include "znpch.hpp"
#include "Texture.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanTexture.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(specification);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, const std::filesystem::path& filepath)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(specification, filepath);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, Buffer imageData)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(specification, imageData);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::CreateFromSRGB(Ref<Texture2D> texture)
	{
		TextureSpecification spec;
		spec.Width = texture->GetWidth();
		spec.Height = texture->GetHeight();
		spec.Format = ImageFormat::SRGBA;
		BufferSafe buffer;
		texture->GetImage()->CopyToHostBuffer(buffer);
		auto srgbTexture = Texture2D::Create(spec, buffer);
		return srgbTexture;
	}

	Ref<TextureCube> TextureCube::Create(const TextureSpecification& specification, Buffer imageData)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTextureCube>::Create(specification, imageData);
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
		

}
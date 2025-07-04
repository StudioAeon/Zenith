#pragma once

#include "Zenith/Renderer/ResourceFactory.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanTexture.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanVertexBuffer.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanIndexBuffer.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanImage.hpp"

namespace Zenith {

	class VulkanResourceFactory : public ResourceFactory
	{
	public:
		Ref<Texture2D> CreateTexture2D(const TextureSpecification& spec) override
		{
			return Ref<VulkanTexture2D>::Create(spec);
		}

		Ref<Texture2D> CreateTexture2D(const TextureSpecification& spec, const std::filesystem::path& filepath) override
		{
			return Ref<VulkanTexture2D>::Create(spec, filepath);
		}

		Ref<Texture2D> CreateTexture2D(const TextureSpecification& spec, Buffer imageData) override
		{
			return Ref<VulkanTexture2D>::Create(spec, imageData);
		}

		Ref<TextureCube> CreateTextureCube(const TextureSpecification& spec, Buffer imageData) override
		{
			return Ref<VulkanTextureCube>::Create(spec, imageData);
		}

		Ref<VertexBuffer> CreateVertexBuffer(void* data, uint64_t size, VertexBufferUsage usage) override
		{
			return Ref<VulkanVertexBuffer>::Create(data, size, usage);
		}

		Ref<VertexBuffer> CreateVertexBuffer(uint64_t size, VertexBufferUsage usage) override
		{
			return Ref<VulkanVertexBuffer>::Create(size, usage);
		}

		Ref<IndexBuffer> CreateIndexBuffer(void* data, uint64_t size) override
		{
			return Ref<VulkanIndexBuffer>::Create(data, size);
		}

		Ref<IndexBuffer> CreateIndexBuffer(uint64_t size) override
		{
			return Ref<VulkanIndexBuffer>::Create(size);
		}

		Ref<Image2D> CreateImage2D(const ImageSpecification& spec, Buffer buffer) override
		{
			return Ref<VulkanImage2D>::Create(spec);
		}
	};

}
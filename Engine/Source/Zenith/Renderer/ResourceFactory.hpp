#pragma once

#include "Zenith/Renderer/Texture.hpp"
#include "Zenith/Renderer/VertexBuffer.hpp"
#include "Zenith/Renderer/IndexBuffer.hpp"
#include "Zenith/Renderer/Image.hpp"
#include "Zenith/Core/Buffer.hpp"

#include <filesystem>

namespace Zenith {

	class ResourceFactory
	{
	public:
		virtual ~ResourceFactory() = default;

		virtual Ref<Texture2D> CreateTexture2D(const TextureSpecification& spec) = 0;
		virtual Ref<Texture2D> CreateTexture2D(const TextureSpecification& spec, const std::filesystem::path& filepath) = 0;
		virtual Ref<Texture2D> CreateTexture2D(const TextureSpecification& spec, Buffer imageData) = 0;
		virtual Ref<TextureCube> CreateTextureCube(const TextureSpecification& spec, Buffer imageData) = 0;

		virtual Ref<VertexBuffer> CreateVertexBuffer(void* data, uint64_t size, VertexBufferUsage usage = VertexBufferUsage::Static) = 0;
		virtual Ref<VertexBuffer> CreateVertexBuffer(uint64_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic) = 0;
		virtual Ref<IndexBuffer> CreateIndexBuffer(void* data, uint64_t size) = 0;
		virtual Ref<IndexBuffer> CreateIndexBuffer(uint64_t size) = 0;

		virtual Ref<Image2D> CreateImage2D(const ImageSpecification& spec, Buffer buffer = Buffer()) = 0;
	};

}
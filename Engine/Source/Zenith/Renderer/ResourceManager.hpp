#pragma once

#include "Zenith/Renderer/ResourceFactory.hpp"
#include "Zenith/Renderer/RendererAPI.hpp"
#include "Zenith/Core/Ref.hpp"

#include <memory>

namespace Zenith {

	class ResourceManager
	{
	public:
		static ResourceManager& Instance()
		{
			static ResourceManager instance;
			return instance;
		}

		void Initialize(RendererAPIType apiType);
		void Shutdown();

		Ref<Texture2D> CreateTexture2D(const TextureSpecification& spec);
		Ref<Texture2D> CreateTexture2D(const TextureSpecification& spec, const std::filesystem::path& filepath);
		Ref<Texture2D> CreateTexture2D(const TextureSpecification& spec, Buffer imageData);
		Ref<TextureCube> CreateTextureCube(const TextureSpecification& spec, Buffer imageData);

		Ref<VertexBuffer> CreateVertexBuffer(void* data, uint64_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		Ref<VertexBuffer> CreateVertexBuffer(uint64_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);
		Ref<IndexBuffer> CreateIndexBuffer(void* data, uint64_t size);
		Ref<IndexBuffer> CreateIndexBuffer(uint64_t size);

		Ref<Image2D> CreateImage2D(const ImageSpecification& spec, Buffer buffer = Buffer());

		ResourceFactory* GetFactory() const { return m_ResourceFactory.get(); }

	private:
		ResourceManager() = default;
		~ResourceManager() = default;

		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;

		std::unique_ptr<ResourceFactory> m_ResourceFactory;
	};

}
#include "znpch.hpp"
#include "ResourceManager.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanResourceFactory.hpp"
#include "Zenith/Renderer/RendererAPI.hpp"

namespace Zenith {

	void ResourceManager::Initialize(RendererAPIType apiType)
	{
		std::make_unique<VulkanResourceFactory>();
	}

	void ResourceManager::Shutdown()
	{
		m_ResourceFactory.reset();
	}

	Ref<Texture2D> ResourceManager::CreateTexture2D(const TextureSpecification& spec)
	{
		ZN_CORE_ASSERT(m_ResourceFactory, "ResourceFactory not initialized");
		return m_ResourceFactory->CreateTexture2D(spec);
	}

	Ref<Texture2D> ResourceManager::CreateTexture2D(const TextureSpecification& spec, const std::filesystem::path& filepath)
	{
		ZN_CORE_ASSERT(m_ResourceFactory, "ResourceFactory not initialized");
		return m_ResourceFactory->CreateTexture2D(spec, filepath);
	}

	Ref<Texture2D> ResourceManager::CreateTexture2D(const TextureSpecification& spec, Buffer imageData)
	{
		ZN_CORE_ASSERT(m_ResourceFactory, "ResourceFactory not initialized");
		return m_ResourceFactory->CreateTexture2D(spec, imageData);
	}

	Ref<TextureCube> ResourceManager::CreateTextureCube(const TextureSpecification& spec, Buffer imageData)
	{
		ZN_CORE_ASSERT(m_ResourceFactory, "ResourceFactory not initialized");
		return m_ResourceFactory->CreateTextureCube(spec, imageData);
	}

	Ref<VertexBuffer> ResourceManager::CreateVertexBuffer(void* data, uint64_t size, VertexBufferUsage usage)
	{
		ZN_CORE_ASSERT(m_ResourceFactory, "ResourceFactory not initialized");
		return m_ResourceFactory->CreateVertexBuffer(data, size, usage);
	}

	Ref<VertexBuffer> ResourceManager::CreateVertexBuffer(uint64_t size, VertexBufferUsage usage)
	{
		ZN_CORE_ASSERT(m_ResourceFactory, "ResourceFactory not initialized");
		return m_ResourceFactory->CreateVertexBuffer(size, usage);
	}

	Ref<IndexBuffer> ResourceManager::CreateIndexBuffer(void* data, uint64_t size)
	{
		ZN_CORE_ASSERT(m_ResourceFactory, "ResourceFactory not initialized");
		return m_ResourceFactory->CreateIndexBuffer(data, size);
	}

	Ref<IndexBuffer> ResourceManager::CreateIndexBuffer(uint64_t size)
	{
		ZN_CORE_ASSERT(m_ResourceFactory, "ResourceFactory not initialized");
		return m_ResourceFactory->CreateIndexBuffer(size);
	}

	Ref<Image2D> ResourceManager::CreateImage2D(const ImageSpecification& spec, Buffer buffer)
	{
		ZN_CORE_ASSERT(m_ResourceFactory, "ResourceFactory not initialized");
		return m_ResourceFactory->CreateImage2D(spec, buffer);
	}

}
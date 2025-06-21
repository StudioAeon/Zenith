#pragma once

#include "Zenith/Renderer/VertexBuffer.hpp"
#include "VulkanBuffer.hpp"

namespace Zenith {

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(const void* data, uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		VulkanVertexBuffer(uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);
		virtual ~VulkanVertexBuffer() = default;

		virtual void SetData(void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void Bind() const override;

		virtual uint32_t GetSize() const override { return m_Buffer->GetSize(); }
		virtual RendererID GetRendererID() const override {
			return static_cast<RendererID>(reinterpret_cast<uintptr_t>(m_Buffer->GetVulkanBuffer()) & 0xFFFFFFFF);
		}

		VkBuffer GetVulkanBuffer() const { return m_Buffer->GetVulkanBuffer(); }

		static Ref<VulkanVertexBuffer> Create(const void* data, uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		static Ref<VulkanVertexBuffer> Create(uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);

	private:
		Ref<VulkanBuffer> m_Buffer;
		Ref<VulkanBuffer> m_StagingBuffer;
		VertexBufferUsage m_Usage;
	};

}
#pragma once

#include "Zenith/Renderer/IndexBuffer.hpp"
#include "VulkanBuffer.hpp"

namespace Zenith {

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(const void* data, uint32_t size);
		VulkanIndexBuffer(uint32_t size);
		virtual ~VulkanIndexBuffer() = default;

		virtual void SetData(void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void Bind() const override;

		virtual uint32_t GetCount() const override { return m_Buffer->GetSize() / sizeof(uint32_t); }
		virtual uint32_t GetSize() const override { return m_Buffer->GetSize(); }
		virtual RendererID GetRendererID() const override {
			return static_cast<RendererID>(reinterpret_cast<uintptr_t>(m_Buffer->GetVulkanBuffer()) & 0xFFFFFFFF);
		}

		VkBuffer GetVulkanBuffer() const { return m_Buffer->GetVulkanBuffer(); }

		static Ref<VulkanIndexBuffer> Create(const void* data, uint32_t size);
		static Ref<VulkanIndexBuffer> Create(uint32_t size);

	private:
		Ref<VulkanBuffer> m_Buffer;
		Ref<VulkanBuffer> m_StagingBuffer;
	};

}
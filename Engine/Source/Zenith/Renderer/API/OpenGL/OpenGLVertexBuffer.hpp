#pragma once

#include "Zenith/Renderer/VertexBuffer.hpp"
#include "Zenith/Core/Buffer.hpp"

namespace Zenith {

	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		OpenGLVertexBuffer(uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);
		virtual ~OpenGLVertexBuffer();

		virtual void SetData(void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void Bind() const override;

		virtual uint32_t GetSize() const override { return m_Size; }
		virtual RendererID GetRendererID() const override { return m_RendererID; }

	private:
		RendererID m_RendererID = 0;
		uint32_t m_Size;
		VertexBufferUsage m_Usage;
		Buffer m_LocalData;
	};

}
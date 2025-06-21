#include "znpch.hpp"
#include "TriangleRenderer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanVertexBuffer.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanIndexBuffer.hpp"

#include <fstream>

namespace Zenith {

	static const TriangleVertex s_TriangleVertices[] = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},   // Top vertex - Red
		{{-0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},   // Bottom left - Green
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}     // Bottom right - Blue
	};

	static const uint32_t s_TriangleIndices[] = { 0, 1, 2 };

	TriangleRenderer::TriangleRenderer()
	{}

	void TriangleRenderer::Initialize()
	{
		if (m_Initialized)
			return;
		CreateTriangleData();
		CreateShaders();
		CreatePipeline();
		m_Initialized = true;
		ZN_CORE_INFO("TriangleRenderer initialized successfully");
	}

	void TriangleRenderer::CreateTriangleData()
	{
		m_VertexBuffer = VertexBuffer::Create(
			const_cast<void*>(static_cast<const void*>(s_TriangleVertices)),
			sizeof(s_TriangleVertices),
			VertexBufferUsage::Static
		);

		m_IndexBuffer = IndexBuffer::Create(
			const_cast<void*>(static_cast<const void*>(s_TriangleIndices)),
			sizeof(s_TriangleIndices)
		);
	}

	void TriangleRenderer::CreateShaders()
	{
		auto vertexSpirv = LoadSpirvFile("Resources/Shaders/triangle.vert.spv");
		auto fragmentSpirv = LoadSpirvFile("Resources/Shaders/triangle.frag.spv");

		if (!vertexSpirv.empty() && !fragmentSpirv.empty())
		{
			m_Shader = VulkanShader::CreateFromSpirv("TriangleShader", vertexSpirv, fragmentSpirv);
			ZN_CORE_INFO("TriangleRenderer: Loaded shaders from SPIR-V files");
		}
	}

	std::vector<uint32_t> TriangleRenderer::LoadSpirvFile(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			ZN_CORE_TRACE("Failed to open SPIR-V file: {}", filepath);
			return {};
		}

		size_t fileSize = (size_t)file.tellg();
		if (fileSize % sizeof(uint32_t) != 0)
		{
			ZN_CORE_ERROR("SPIR-V file size is not a multiple of 4 bytes: {}", filepath);
			file.close();
			return {};
		}

		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
		file.close();

		ZN_CORE_TRACE("Loaded SPIR-V file: {} ({} bytes)", filepath, fileSize);
		return buffer;
	}

	void TriangleRenderer::CreatePipeline()
	{
		auto context = VulkanContext::Get();
		VkRenderPass renderPass = context->GetSwapChain().GetRenderPass();
		PipelineSpecification spec;
		spec.shader = m_Shader;
		spec.vertexInput = TriangleVertex::GetDescription();
		spec.renderPass = renderPass;
		spec.wireframe = false;
		spec.depthTest = false;  // No depth testing for 2D triangle
		spec.cullMode = VK_CULL_MODE_NONE;
		m_Pipeline = VulkanPipeline::Create(spec);
	}

	void TriangleRenderer::Render(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height)
	{
		if (!m_Initialized)
			return;

		// Set viewport
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(width);
		viewport.height = static_cast<float>(height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Set scissor
		VkRect2D scissor = {};
		scissor.offset = {0, 0};
		scissor.extent = {width, height};
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// Bind pipeline
		m_Pipeline->Bind(commandBuffer);

		auto vulkanVertexBuffer = m_VertexBuffer.As<VulkanVertexBuffer>();
		auto vulkanIndexBuffer = m_IndexBuffer.As<VulkanIndexBuffer>();

		ZN_CORE_ASSERT(vulkanVertexBuffer && vulkanIndexBuffer, "Expected Vulkan buffer types");

		VkBuffer vertexBuffers[] = { vulkanVertexBuffer->GetVulkanBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, vulkanIndexBuffer->GetCount(), 1, 0, 0, 0);
	}

	void TriangleRenderer::Shutdown()
	{
		m_Pipeline.Reset();
		m_Shader.Reset();
		m_IndexBuffer.Reset();
		m_VertexBuffer.Reset();
		m_Initialized = false;
	}

	Ref<TriangleRenderer> TriangleRenderer::Create()
	{
		return Ref<TriangleRenderer>::Create();
	}

}
#pragma once

#include "Zenith/Core/Ref.hpp"

#include "Zenith/Renderer/VertexBuffer.hpp"
#include "Zenith/Renderer/IndexBuffer.hpp"

#include "Zenith/Renderer/API/Vulkan/VulkanShader.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanPipeline.hpp"

namespace Zenith {

	struct TriangleVertex
	{
		float position[2];  // x, y
		float color[3];     // r, g, b

		static VertexInputDescription GetDescription()
		{
			VertexInputDescription description;

			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(TriangleVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			description.bindings.push_back(bindingDescription);

			VkVertexInputAttributeDescription positionAttribute = {};
			positionAttribute.binding = 0;
			positionAttribute.location = 0;
			positionAttribute.format = VK_FORMAT_R32G32_SFLOAT;
			positionAttribute.offset = offsetof(TriangleVertex, position);
			description.attributes.push_back(positionAttribute);

			VkVertexInputAttributeDescription colorAttribute = {};
			colorAttribute.binding = 0;
			colorAttribute.location = 1;
			colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
			colorAttribute.offset = offsetof(TriangleVertex, color);
			description.attributes.push_back(colorAttribute);

			return description;
		}
	};

	class TriangleRenderer : public RefCounted
	{
	public:
		TriangleRenderer();
		~TriangleRenderer() = default;

		void Initialize();
		void Render(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height);
		void Shutdown();

		static Ref<TriangleRenderer> Create();

	private:
		void CreateTriangleData();
		void CreateShaders();
		void CreatePipeline();

		std::vector<uint32_t> LoadSpirvFile(const std::string& filepath);

	private:
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		Ref<VulkanShader> m_Shader;
		Ref<VulkanPipeline> m_Pipeline;

		bool m_Initialized = false;
	};

}
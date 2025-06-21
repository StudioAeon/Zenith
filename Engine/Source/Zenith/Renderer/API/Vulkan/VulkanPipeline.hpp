#pragma once

#include "Zenith/Core/Ref.hpp"
#include "Vulkan.hpp"
#include "VulkanDevice.hpp"
#include "VulkanShader.hpp"
#include <vector>

namespace Zenith {

	struct VertexInputDescription
	{
		std::vector<VkVertexInputBindingDescription> bindings;
		std::vector<VkVertexInputAttributeDescription> attributes;
	};

	struct PipelineSpecification
	{
		Ref<VulkanShader> shader;
		VertexInputDescription vertexInput;
		VkRenderPass renderPass;
		bool wireframe = false;
		bool depthTest = true;
		bool depthWrite = true;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	};

	class VulkanPipeline : public RefCounted
	{
	public:
		VulkanPipeline(const PipelineSpecification& spec);
		virtual ~VulkanPipeline();

		void Bind(VkCommandBuffer commandBuffer);

		VkPipeline GetVulkanPipeline() const { return m_Pipeline; }
		VkPipelineLayout GetLayout() const { return m_PipelineLayout; }

		static Ref<VulkanPipeline> Create(const PipelineSpecification& spec);

	private:
		void CreatePipelineLayout();
		void CreatePipeline();

	private:
		PipelineSpecification m_Specification;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		Ref<VulkanDevice> m_Device;
	};
}
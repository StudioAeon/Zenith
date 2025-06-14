#include "znpch.hpp"
#include "VulkanRenderer.hpp"

#include "Vulkan.hpp"
#include "VulkanContext.hpp"

#include "Zenith/Renderer/Renderer.hpp"

namespace Zenith {

	struct VulkanRendererData
	{
		RendererCapabilities RenderCaps;
		VkCommandBuffer ActiveCommandBuffer = nullptr;
	};

	static VulkanRendererData* s_Data = nullptr;

	void VulkanRenderer::Init()
	{
		s_Data = new VulkanRendererData();

		auto context = VulkanContext::Get();
		auto physicalDevice = context->GetDevice()->GetPhysicalDevice();
		auto& caps = s_Data->RenderCaps;

		// Basic capabilities setup
		caps.Vendor = "Vulkan";
		caps.Device = "Unknown";
		caps.Version = "1.2";
		caps.MaxSamples = 1;
		caps.MaxAnisotropy = 1.0f;
		caps.MaxTextureUnits = 32;
	}

	void VulkanRenderer::Shutdown()
	{
		delete s_Data;
		s_Data = nullptr;
	}

	RendererCapabilities& VulkanRenderer::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}

	void VulkanRenderer::BeginFrame()
	{
		Renderer::Submit([]()
		{
			Ref<VulkanContext> context = VulkanContext::Get();
			VulkanSwapChain& swapChain = context->GetSwapChain();

			VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
			s_Data->ActiveCommandBuffer = drawCommandBuffer;
			ZN_CORE_ASSERT(s_Data->ActiveCommandBuffer);

			VK_CHECK_RESULT(vkResetCommandBuffer(s_Data->ActiveCommandBuffer, 0));

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			VK_CHECK_RESULT(vkBeginCommandBuffer(s_Data->ActiveCommandBuffer, &cmdBufInfo));

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
			renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();
			renderPassBeginInfo.renderArea.offset = {0, 0};
			renderPassBeginInfo.renderArea.extent = {swapChain.GetWidth(), swapChain.GetHeight()};

			VkClearValue clearColor = {{{0.0f, 0.2f, 0.4f, 1.0f}}};
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(s_Data->ActiveCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		});
	}

	void VulkanRenderer::EndFrame()
	{
		Renderer::Submit([]()
		{
			ZN_CORE_ASSERT(s_Data->ActiveCommandBuffer);

			vkCmdEndRenderPass(s_Data->ActiveCommandBuffer);

			VK_CHECK_RESULT(vkEndCommandBuffer(s_Data->ActiveCommandBuffer));
			s_Data->ActiveCommandBuffer = nullptr;
		});
	}

}
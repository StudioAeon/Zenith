#include "znpch.hpp"
#include "VulkanRenderer.hpp"

#include "Vulkan.hpp"
#include "VulkanContext.hpp"
#include "TriangleRenderer.hpp"

#include "Zenith/Renderer/API/Vulkan/DXCCompiler.hpp"

#include "Zenith/Renderer/Renderer.hpp"

namespace Zenith {

	struct VulkanRendererData
	{
		RendererCapabilities RenderCaps;
		VkCommandBuffer ActiveCommandBuffer = nullptr;
		Ref<TriangleRenderer> TriangleRenderer;
	};

	static VulkanRendererData* s_Data = nullptr;

	void VulkanRenderer::Init()
	{
		s_Data = new VulkanRendererData();
		auto context = VulkanContext::Get();
		auto device = context->GetDevice();
		auto physicalDevice = device->GetPhysicalDevice();
		auto& caps = s_Data->RenderCaps;

		VkPhysicalDevice vkPhysicalDevice = physicalDevice->GetVulkanPhysicalDevice();
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(vkPhysicalDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &deviceFeatures);

		caps.Device = deviceProperties.deviceName;
		caps.Version = std::to_string(VK_VERSION_MAJOR(deviceProperties.apiVersion)) + "." +
					   std::to_string(VK_VERSION_MINOR(deviceProperties.apiVersion)) + "." +
					   std::to_string(VK_VERSION_PATCH(deviceProperties.apiVersion));

		switch (deviceProperties.vendorID) {
			case 0x1002: caps.Vendor = "AMD"; break;
			case 0x1010: caps.Vendor = "ImgTec"; break;
			case 0x10DE: caps.Vendor = "NVIDIA"; break;
			case 0x13B5: caps.Vendor = "ARM"; break;
			case 0x5143: caps.Vendor = "Qualcomm"; break;
			case 0x8086: caps.Vendor = "Intel"; break;
			default: caps.Vendor = "Unknown (0x" + std::to_string(deviceProperties.vendorID) + ")"; break;
		}

		caps.MaxSamples = static_cast<int>(deviceProperties.limits.framebufferColorSampleCounts &
											deviceProperties.limits.framebufferDepthSampleCounts);
		caps.MaxAnisotropy = deviceFeatures.samplerAnisotropy ? deviceProperties.limits.maxSamplerAnisotropy : 1.0f;
		caps.MaxTextureUnits = static_cast<int>(deviceProperties.limits.maxBoundDescriptorSets);

		Utils::DumpGPUInfo();

		auto& dxcCompiler = DXCCompiler::Get();
		if (!dxcCompiler.Initialize())
		{
			ZN_CORE_WARN("Failed to initialize DXC compiler - falling back to pre-compiled shaders");
		}

		// Initialize triangle renderer
		s_Data->TriangleRenderer = TriangleRenderer::Create();
		s_Data->TriangleRenderer->Initialize();
	}

	void VulkanRenderer::Shutdown()
	{
		if (s_Data && s_Data->TriangleRenderer)
		{
			s_Data->TriangleRenderer->Shutdown();
			s_Data->TriangleRenderer.Reset();
		}

		DXCCompiler::Get().Shutdown();

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

			VkClearValue clearColor = {{{0.0f, 0.1f, 0.2f, 1.0f}}};
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(s_Data->ActiveCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Render triangle
			if (s_Data->TriangleRenderer)
			{
				s_Data->TriangleRenderer->Render(
					s_Data->ActiveCommandBuffer,
					swapChain.GetWidth(),
					swapChain.GetHeight()
				);
			}
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
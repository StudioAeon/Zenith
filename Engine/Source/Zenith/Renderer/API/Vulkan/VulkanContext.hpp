#pragma once

#include "Zenith/Renderer/RendererContext.hpp"

#include "Vulkan.hpp"
#include "VulkanDevice.hpp"
#include "VulkanAllocator.hpp"
#include "VulkanSwapChain.hpp"

struct SDL_Window;

namespace Zenith {

	class VulkanContext : public RendererContext
	{
	public:
		VulkanContext(SDL_Window* windowHandle);
		virtual ~VulkanContext();

		virtual void Create() override;
		virtual void SwapBuffers() override;
		virtual void OnResize(uint32_t width, uint32_t height) override;
		virtual void BeginFrame() override;

		Ref<VulkanDevice> GetDevice() { return m_Device; }
		VulkanSwapChain& GetSwapChain() { return m_SwapChain; }

		static VkInstance GetInstance() { return s_VulkanInstance; }

		static Ref<VulkanContext> Get();
		static Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetDevice(); }

	private:
		SDL_Window* m_WindowHandle;

		// Devices
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		Ref<VulkanDevice> m_Device;

		// Vulkan instance
		inline static VkInstance s_VulkanInstance = VK_NULL_HANDLE;
		VkDebugReportCallbackEXT m_DebugReportCallback = VK_NULL_HANDLE;
		VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;

		VulkanAllocator m_Allocator;
		VulkanSwapChain m_SwapChain;
	};
}
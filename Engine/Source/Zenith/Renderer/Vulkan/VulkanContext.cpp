#include "znpch.hpp"
#include "VulkanContext.hpp"
#include "Vulkan.hpp"
#include "Zenith/Renderer/API/Renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace Zenith::Utils {
	inline bool IsVulkanAvailable()
	{
		if (!SDL_Vulkan_LoadLibrary(nullptr))
		{
			return false;
		}

		uint32_t extensionCount = 0;
		const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

		if (!extensions || extensionCount == 0)
		{
			SDL_Vulkan_UnloadLibrary();
			return false;
		}

		return true;
	}
}

namespace Zenith {

#ifdef ZN_DEBUG
	static bool s_Validation = true;
#else
	static bool s_Validation = false;
#endif

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
		(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
		ZN_CORE_WARN("VulkanDebugCallback:\n  Object Type: {0}\n  Message: {1}", static_cast<uint32_t>(objectType), pMessage);
		return VK_FALSE;
	}

	VulkanContext::VulkanContext(SDL_Window* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}

	VulkanContext::~VulkanContext()
	{
		m_SwapChain.Cleanup();
		m_Device->Destroy();

		vkDestroyInstance(s_VulkanInstance, nullptr);
		s_VulkanInstance = VK_NULL_HANDLE;
	}

	void VulkanContext::Create()
	{
		ZN_CORE_INFO_TAG("Renderer", "VulkanContext::Create");

		if (!SDL_Vulkan_LoadLibrary(nullptr))
		{
			const char* error = SDL_GetError();
			ZN_CORE_ERROR_TAG("Renderer", "Failed to load Vulkan library: {}", error ? error : "Unknown error");
			ZN_CORE_ERROR_TAG("Renderer", "Make sure Vulkan drivers are installed on your system");
			ZN_CORE_ASSERT(false, "Vulkan is not available");
			return;
		}

		ZN_CORE_INFO_TAG("Renderer", "Vulkan library loaded successfully");

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Application Info
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Zenith";
		appInfo.pEngineName = "Zenith";
		appInfo.apiVersion = VK_API_VERSION_1_2;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Extensions and Validation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Get required extensions from SDL
		uint32_t extensionCount = 0;
		const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
		if (!extensions)
		{
			ZN_CORE_ASSERT(false, "Failed to get Vulkan instance extensions: {}", SDL_GetError());
		}

		std::vector<const char*> instanceExtensions(extensions, extensions + extensionCount);

		if (s_Validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		// TODO: Extract all validation into separate class
		if (s_Validation)
		{
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
			uint32_t instanceLayerCount;
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
			std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
			bool validationLayerPresent = false;
			ZN_CORE_TRACE("Vulkan Instance Layers:");
			for (const VkLayerProperties& layer : instanceLayerProperties)
			{
				ZN_CORE_TRACE("  {0}", layer.layerName);
				if (strcmp(layer.layerName, validationLayerName) == 0)
				{
					validationLayerPresent = true;
					break;
				}
			}
			if (validationLayerPresent)
			{
				instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else
			{
				ZN_CORE_ERROR("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Instance and Surface Creation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &s_VulkanInstance));

		if (s_Validation)
		{
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(s_VulkanInstance, "vkCreateDebugReportCallbackEXT");
			ZN_CORE_ASSERT(vkCreateDebugReportCallbackEXT != nullptr, "Failed to get debug report callback function");
			VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
			debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			debug_report_ci.pfnCallback = VulkanDebugReportCallback;
			debug_report_ci.pUserData = nullptr;
			VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(s_VulkanInstance, &debug_report_ci, nullptr, &m_DebugReportCallback));
		}

		m_PhysicalDevice = VulkanPhysicalDevice::Select();

		VkPhysicalDeviceFeatures enabledFeatures;
		memset(&enabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
		enabledFeatures.samplerAnisotropy = true;
		enabledFeatures.robustBufferAccess = true;
		m_Device = Ref<VulkanDevice>::Create(m_PhysicalDevice, enabledFeatures);

		m_Allocator = VulkanAllocator(m_Device, "Default");

		m_SwapChain.Init(s_VulkanInstance, m_Device);
		m_SwapChain.InitSurface(m_WindowHandle);

		uint32_t width = 1280, height = 720;
		m_SwapChain.Create(&width, &height);

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(m_Device->GetVulkanDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}

	void VulkanContext::OnResize(uint32_t width, uint32_t height)
	{
		m_SwapChain.OnResize(width, height);
	}

	void VulkanContext::BeginFrame()
	{
		m_SwapChain.BeginFrame();
	}

	void VulkanContext::SwapBuffers()
	{
		m_SwapChain.Present();
	}

	// Implementation moved here to avoid circular dependency
	Ref<VulkanContext> VulkanContext::Get()
	{
		auto context = Renderer::GetContext();
		return context.As<VulkanContext>();
	}

}
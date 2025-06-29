#include "znpch.hpp"
#include "VulkanContext.hpp"

#include "Vulkan.hpp"
#include "VulkanImage.hpp"

#include <SDL3/SDL_vulkan.h>

#ifdef ZN_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include <format>

#ifndef VK_API_VERSION_1_2
#error Wrong Vulkan SDK! Please run scripts/Setup.bat
#endif


namespace Zenith {

#if defined(ZN_DEBUG) || defined(ZN_RELEASE)
	static bool s_Validation = true;
#else
	static bool s_Validation = false; // Let's leave this on for now...
#endif

#if 0
	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
		(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
		ZN_CORE_WARN_TAG("Renderer", "VulkanDebugCallback:\n  Object Type: {0}\n  Message: {1}", objectType, pMessage);

		const auto& imageRefs = VulkanImage2D::GetImageRefs();
		if (strstr(pMessage, "CoreValidation-DrawState-InvalidImageLayout"))
			ZN_CORE_ASSERT(false);

		return VK_FALSE;
	}
#endif

	constexpr const char* VkDebugUtilsMessageType(const VkDebugUtilsMessageTypeFlagsEXT type)
	{
		switch (type)
		{
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:		return "General";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:	return "Validation";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:	return "Performance";
			default:												return "Unknown";
		}
	}

	constexpr const char* VkDebugUtilsMessageSeverity(const VkDebugUtilsMessageSeverityFlagBitsEXT severity)
	{
		switch (severity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:		return "error";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:	return "warning";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:		return "info";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:	return "verbose";
			default:												return "unknown";
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugUtilsMessengerCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		(void)pUserData; //Unused argument

		const bool performanceWarnings = false;
		if (!performanceWarnings)
		{
			if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
				return VK_FALSE;
		}

		std::string labels, objects;
		if (pCallbackData->cmdBufLabelCount)
		{
			labels = std::format("\tLabels({}): \n", pCallbackData->cmdBufLabelCount);
			for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
			{
				const auto& label = pCallbackData->pCmdBufLabels[i];
				const std::string colorStr = std::format("[ {}, {}, {}, {} ]", label.color[0], label.color[1], label.color[2], label.color[3]);
				labels.append(std::format("\t\t- Command Buffer Label[{0}]: name: {1}, color: {2}\n", i, label.pLabelName ? label.pLabelName : "NULL", colorStr));
			}
		}

		if (pCallbackData->objectCount)
		{
			objects = std::format("\tObjects({}): \n", pCallbackData->objectCount);
			for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
			{
				const auto& object = pCallbackData->pObjects[i];
				objects.append(std::format("\t\t- Object[{0}] name: {1}, type: {2}, handle: {3:#x}\n", i, object.pObjectName ? object.pObjectName : "NULL", Utils::VkObjectTypeToString(object.objectType), object.objectHandle));
			}
		}

		printf("%s %s message: \n\t%s\n %s %s\n",
			VkDebugUtilsMessageType(messageType),
			VkDebugUtilsMessageSeverity(messageSeverity),
			pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "No message",
			labels.empty() ? "No labels" : labels.c_str(),
			objects.empty() ? "No objects" : objects.c_str());
		[[maybe_unused]] const auto& imageRefs = VulkanImage2D::GetImageRefs();

		return VK_FALSE;
	}

	static bool CheckDriverAPIVersionSupport(uint32_t minimumSupportedVersion)
	{
		uint32_t instanceVersion;
		vkEnumerateInstanceVersion(&instanceVersion);

		if (instanceVersion < minimumSupportedVersion)
		{
			ZN_CORE_FATAL("Incompatible Vulkan driver version!");
			ZN_CORE_FATAL("  You have {}.{}.{}", VK_API_VERSION_MAJOR(instanceVersion), VK_API_VERSION_MINOR(instanceVersion), VK_API_VERSION_PATCH(instanceVersion));
			ZN_CORE_FATAL("  You need at least {}.{}.{}", VK_API_VERSION_MAJOR(minimumSupportedVersion), VK_API_VERSION_MINOR(minimumSupportedVersion), VK_API_VERSION_PATCH(minimumSupportedVersion));

			return false;
		}

		return true;
	}

	VulkanContext::VulkanContext()
	{
	}

	VulkanContext::~VulkanContext()
	{
		// Its too late to destroy the device here, because Destroy() asks for the context (which we're in the middle of destructing)
		// Device is destroyed in SDL_Window::Shutdown()
		//m_Device->Destroy();

		vkDestroyInstance(s_VulkanInstance, nullptr);
		s_VulkanInstance = nullptr;
	}

	void VulkanContext::Init()
	{
		ZN_CORE_INFO_TAG("Renderer", "VulkanContext::Create");

		if (SDL_Vulkan_LoadLibrary(nullptr) != 0) {
			const char* sdlError = SDL_GetError();
			ZN_CORE_WARN("SDL_Vulkan_LoadLibrary failed: {}",
				sdlError && strlen(sdlError) > 0 ? sdlError : "Unknown SDL error");

			SDL_ClearError();

			ZN_CORE_INFO("Attempting to continue with potentially static Vulkan linkage...");
		}

		if (!CheckDriverAPIVersionSupport(VK_API_VERSION_1_2))
		{
#ifdef ZN_PLATFORM_WINDOWS
			MessageBox(nullptr, reinterpret_cast<LPCSTR>(L"Incompatible Vulkan driver version.\nUpdate your GPU drivers!"), reinterpret_cast<LPCSTR>(L"Zenith Error"), MB_OK | MB_ICONERROR);
#else
			ZN_CORE_ERROR("Incompatible Vulkan driver version.\nUpdate your GPU drivers!");
#endif
			ZN_CORE_VERIFY(false);
		}

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
		// Let SDL3 determine required Vulkan extensions
		unsigned int extensionCount = 0;
		const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
		if (!extensions) {
			ZN_CORE_ERROR("Failed to get Vulkan instance extensions: {}", SDL_GetError());
			return;
		}

		std::vector<const char*> instanceExtensions;
		for (unsigned int i = 0; i < extensionCount; i++) {
			instanceExtensions.push_back(extensions[i]);
		}

		ZN_CORE_INFO("SDL3 required Vulkan extensions:");
		for (const char* ext : instanceExtensions) {
			ZN_CORE_INFO("  - {}", ext);
		}
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Very little performance hit, can be used in Release.
		if (s_Validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}

		VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
		VkValidationFeaturesEXT features = {};
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		features.enabledValidationFeatureCount = 1;
		features.pEnabledValidationFeatures = enables;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr; // &features;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		// TODO: Extract all validation into separate class
		if (s_Validation)
		{
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
			// Check if this layer is available at instance level
			uint32_t instanceLayerCount;
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
			std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
			bool validationLayerPresent = false;
			ZN_CORE_INFO_TAG("Renderer", "Vulkan Instance Layers:");
			for (const VkLayerProperties& layer : instanceLayerProperties)
			{
				ZN_CORE_INFO_TAG("Renderer", "  {0}", layer.layerName);
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
				ZN_CORE_ERROR_TAG("Renderer", "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Instance and Surface Creation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &s_VulkanInstance));
		Utils::VulkanLoadDebugUtilsExtensions(s_VulkanInstance);

		if (s_Validation)
		{
			auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_VulkanInstance, "vkCreateDebugUtilsMessengerEXT");
			ZN_CORE_ASSERT(vkCreateDebugUtilsMessengerEXT != NULL, "");
			VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo{};
			debugUtilsCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugUtilsCreateInfo.pfnUserCallback = VulkanDebugUtilsMessengerCallback;
			debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT /*  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT*/;

			VK_CHECK_RESULT(vkCreateDebugUtilsMessengerEXT(s_VulkanInstance, &debugUtilsCreateInfo, nullptr, &m_DebugUtilsMessenger));
		}

		m_PhysicalDevice = VulkanPhysicalDevice::Select();

		VkPhysicalDeviceFeatures enabledFeatures;
		memset(&enabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
		enabledFeatures.samplerAnisotropy = true;
		enabledFeatures.wideLines = true;
		enabledFeatures.fillModeNonSolid = true;
		enabledFeatures.independentBlend = true;
		enabledFeatures.pipelineStatisticsQuery = true;
		enabledFeatures.shaderStorageImageReadWithoutFormat = true;
		m_Device = Ref<VulkanDevice>::Create(m_PhysicalDevice, enabledFeatures);

		VulkanAllocator::Init(m_Device);

		// Pipeline Cache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(m_Device->GetVulkanDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}

}

#include "znpch.hpp"
#include "VulkanImGuiLayer.hpp"

#include <imgui/imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include "Zenith/Core/ApplicationContext.hpp"
#include "Zenith/Core/Window.hpp"
#include <SDL3/SDL.h>

#include "Zenith/Renderer/Renderer.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanContext.hpp"

namespace Zenith {

	static void check_vk_result(VkResult err)
	{
		if (err == 0) return;
		fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
		if (err < 0) abort();
	}

	VulkanImGuiLayer::VulkanImGuiLayer(ApplicationContext& context)
		: ImGuiLayer(context)
	{}

	VulkanImGuiLayer::~VulkanImGuiLayer()
	{}

	void VulkanImGuiLayer::OnAttach()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		io.Fonts->AddFontFromFileTTF("Resources/Fonts/Roboto/Roboto-Regular.ttf", 18.0f);
		io.FontDefault = io.Fonts->Fonts.back();

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

		Window& window = GetContext().GetWindow();
		SDL_Window* sdlWindow = static_cast<SDL_Window*>(window.GetNativeWindow());

		if (!sdlWindow) {
			ZN_CORE_ERROR("SDL Window is null!");
			return;
		}

		if (!ImGui_ImplSDL3_InitForVulkan(sdlWindow)) {
			ZN_CORE_ERROR("Failed to initialize ImGui SDL3 backend!");
			return;
		}

		auto vulkanContext = VulkanContext::Get();
		if (!vulkanContext) {
			ZN_CORE_ERROR("Failed to get Vulkan context!");
			return;
		}

		auto device = vulkanContext->GetDevice();
		VkDevice vkDevice = device->GetVulkanDevice();

		VkDescriptorPool descriptorPool;
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		auto err = vkCreateDescriptorPool(vkDevice, &pool_info, nullptr, &descriptorPool);
		check_vk_result(err);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VulkanContext::GetInstance();
		init_info.PhysicalDevice = device->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		init_info.Device = vkDevice;
		init_info.QueueFamily = device->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
		init_info.Queue = device->GetQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;  // No pipeline cache available from context
		init_info.DescriptorPool = descriptorPool;
		init_info.RenderPass = vulkanContext->GetSwapChain().GetRenderPass();
		init_info.Subpass = 0;
		init_info.MinImageCount = 2;
		init_info.ImageCount = vulkanContext->GetSwapChain().GetImageCount();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = check_vk_result;

		ImGui_ImplVulkan_Init(&init_info);
	}

	void VulkanImGuiLayer::OnDetach()
	{
		auto vulkanContext = VulkanContext::Get();
		if (vulkanContext)
		{
			vkDeviceWaitIdle(vulkanContext->GetDevice()->GetVulkanDevice());
		}

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}

	void VulkanImGuiLayer::Begin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
	}

	void VulkanImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();

		Window& window = GetContext().GetWindow();
		io.DisplaySize = ImVec2(static_cast<float>(window.GetWidth()), static_cast<float>(window.GetHeight()));

		// Rendering
		ImGui::Render();

		auto vulkanContext = VulkanContext::Get();
		if (!vulkanContext) return;

		VulkanSwapChain& swapChain = vulkanContext->GetSwapChain();

		VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();

		ImDrawData* main_draw_data = ImGui::GetDrawData();
		if (main_draw_data->DisplaySize.x > 0.0f && main_draw_data->DisplaySize.y > 0.0f)
		{
			ImGui_ImplVulkan_RenderDrawData(main_draw_data, drawCommandBuffer);
		}

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

}
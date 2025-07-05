#include "znpch.hpp"
#include "ImGuiLayer.hpp"
#include "Zenith/Core/ApplicationContext.hpp"
#include "Zenith/Renderer/RendererAPI.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanImGuiLayer.hpp"

namespace Zenith {

	ImGuiLayer::ImGuiLayer(ApplicationContext& context)
		: Layer("ImGuiLayer"), m_Context(context)
	{
	}

	std::shared_ptr<ImGuiLayer> ImGuiLayer::Create(ApplicationContext& context)
	{
		return std::make_shared<VulkanImGuiLayer>(context);
	}

	void ImGuiLayer::AllowInputEvents(bool allowEvents)
	{
		// TODO
		// g_DisableImGuiEvents = !allowEvents;
	}

}
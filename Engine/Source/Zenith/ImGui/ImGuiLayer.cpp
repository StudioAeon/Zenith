#include "znpch.hpp"
#include "ImGuiLayer.hpp"
#include "Zenith/Core/ApplicationContext.hpp"
#include "Zenith/Renderer/RendererAPI.hpp"
// #include "Zenith/Renderer/API/Vulkan/VulkanImGuiLayer.hpp"

namespace Zenith {

	ImGuiLayer::ImGuiLayer(ApplicationContext& context)
		: Layer("ImGuiLayer"), m_Context(context)
	{
	}

	std::shared_ptr<ImGuiLayer> ImGuiLayer::Create(ApplicationContext& context)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:
				ZN_CORE_ASSERT(false, "RendererAPIType::None is currently not supported!");
				return nullptr;

			// case RendererAPIType::Vulkan:
				// return std::make_shared<VulkanImGuiLayer>(context);
		}

		ZN_CORE_ASSERT(false, "Unknown RendererAPIType!");
		return nullptr;
	}

}
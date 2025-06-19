#include "znpch.hpp"
#include "ImGuiLayer.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "Zenith/Renderer/API/OpenGL/OpenGLImGuiLayer.hpp"
#include "Zenith/Renderer/API/Vulkan/VulkanImGuiLayer.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

#include <imgui/imgui.h>

// TODO(Robert): WIP
// Defined in imgui_impl_sdl.cpp
// extern bool g_DisableImGuiEvents;

namespace Zenith {
	 
	std::shared_ptr<ImGuiLayer> ImGuiLayer::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::OpenGL:  return std::make_shared<OpenGLImGuiLayer>();
			case RendererAPIType::Vulkan:  return std::make_shared<VulkanImGuiLayer>();
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void ImGuiLayer::AllowInputEvents(bool allowEvents)
	{
		// TODO
		// g_DisableImGuiEvents = !allowEvents;
	}

}
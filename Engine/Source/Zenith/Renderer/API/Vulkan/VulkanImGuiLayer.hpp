#pragma once

#include "Zenith/ImGui/ImGuiLayer.hpp"
#include "Zenith/Renderer/RenderCommandBuffer.hpp"

#include "backends/imgui_impl_vulkan.h"

namespace Zenith {

	class VulkanImGuiLayer : public ImGuiLayer
	{
	public:
		VulkanImGuiLayer(ApplicationContext& context);
		VulkanImGuiLayer(const std::string& name);
		virtual ~VulkanImGuiLayer();

		virtual void Begin() override;
		virtual void End() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;
	private:
		Ref<RenderCommandBuffer> m_RenderCommandBuffer;
		float m_Time = 0.0f;
	};

}
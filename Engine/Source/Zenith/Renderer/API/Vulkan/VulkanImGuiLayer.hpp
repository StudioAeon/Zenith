#pragma once

#include "Zenith/ImGui/ImGuiLayer.hpp"

namespace Zenith {

	class VulkanImGuiLayer : public ImGuiLayer
	{
	public:
		explicit VulkanImGuiLayer(ApplicationContext& context);
		virtual ~VulkanImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Begin() override;
		virtual void End() override;
	};

}
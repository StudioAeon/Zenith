#pragma once

#include "Zenith/ImGui/ImGuiLayer.hpp"

namespace Zenith {

	class OpenGLImGuiLayer : public ImGuiLayer
	{
	public:
		explicit OpenGLImGuiLayer(ApplicationContext& context);
		virtual ~OpenGLImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Begin() override;
		virtual void End() override;
	};

}
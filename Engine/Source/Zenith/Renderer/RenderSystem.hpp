#pragma once

#include "Zenith/Core/Base.hpp"
#include "Zenith/Core/Window.hpp"

namespace Zenith {

	class LayerStack;
	class ImGuiLayer;
	
	class RenderSystem
	{
	public:
		RenderSystem() = default;
		~RenderSystem() = default;

		void Initialize();
		void Shutdown();

		void BeginFrame();
		void EndFrame();

		void RenderLayers(LayerStack& layerStack);
		void RenderImGui(std::function<void()> imguiRenderFunc, ImGuiLayer* imguiLayer);

		void Present(Window& window);

	private:
		bool m_Initialized = false;
	};

}
#include "znpch.hpp"
#include "RenderSystem.hpp"

#include "Zenith/Core/LayerStack.hpp"
#include "Zenith/ImGui/ImGuiLayer.hpp"
#include "Zenith/Renderer/Renderer.hpp"

namespace Zenith {

	void RenderSystem::Initialize()
	{
		ZN_CORE_ASSERT(!m_Initialized, "RenderSystem already initialized");
		m_Initialized = true;
	}

	void RenderSystem::Shutdown()
	{
		if (!m_Initialized)
			return;
			
		m_Initialized = false;
	}

	void RenderSystem::BeginFrame()
	{
		ZN_CORE_ASSERT(m_Initialized, "RenderSystem not initialized");
		Renderer::BeginFrame();
	}

	void RenderSystem::EndFrame()
	{
		ZN_CORE_ASSERT(m_Initialized, "RenderSystem not initialized");
		Renderer::EndFrame();
	}

	void RenderSystem::RenderLayers(LayerStack& layerStack)
	{
		// Note: Layer::OnUpdate() calls should happen in Application
		// This method would be for actual rendering commands submitted by layers
	}

	void RenderSystem::RenderImGui(std::function<void()> imguiRenderFunc, ImGuiLayer* imguiLayer)
	{
		if (!imguiRenderFunc || !imguiLayer)
			return;

		Renderer::Submit([imguiRenderFunc]() { 
			imguiRenderFunc(); 
		});
		
		Renderer::Submit([imguiLayer]() { 
			imguiLayer->End(); 
		});
	}

	void RenderSystem::Present(Window& window)
	{
		window.GetRenderContext()->BeginFrame();

		Renderer::WaitAndRender();

		Renderer::Submit([&window]() {
			window.SwapBuffers();
		});

		Renderer::SwapQueues();
	}

}
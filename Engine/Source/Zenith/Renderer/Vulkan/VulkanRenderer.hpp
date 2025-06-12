#pragma once

#include "Zenith/Renderer/API/RendererAPI.hpp"

namespace Zenith {

	class VulkanRenderer : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual RendererCapabilities& GetCapabilities() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;
	};

}
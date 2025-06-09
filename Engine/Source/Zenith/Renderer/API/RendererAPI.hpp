#pragma once

#include "RendererTypes.hpp"
#include "RendererCapabilities.hpp"

namespace Zenith {

	enum class RendererAPIType
	{
		None,
		OpenGL,
		Vulkan
	};

	class RendererAPI
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual RendererCapabilities& GetCapabilities() = 0;

		static RendererAPIType Current() { return s_CurrentRendererAPI; }
		static void SetAPI(RendererAPIType api);
	private:
		inline static RendererAPIType s_CurrentRendererAPI = RendererAPIType::OpenGL;
	};

}

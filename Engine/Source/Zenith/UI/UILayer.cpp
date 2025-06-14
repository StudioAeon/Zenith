#include "znpch.hpp"
#include "UILayer.hpp"

#include "Zenith/Renderer/Renderer.hpp"

#include "Zenith/Renderer/RendererAPI.hpp"

// TODO(Robert): WIP

namespace Zenith {

	std::shared_ptr<UILayer> UILayer::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
		}
		ZN_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void UILayer::AllowInputEvents(bool allowEvents)
	{
		// TODO
	}

}
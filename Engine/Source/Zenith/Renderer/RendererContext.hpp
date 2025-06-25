#pragma once

#include "Zenith/Core/Ref.hpp"

struct SDL_Window;

namespace Zenith {

	class RendererContext : public RefCounted
	{
	public:
		RendererContext() = default;
		virtual ~RendererContext() = default;

		virtual void Init() = 0;

		static Ref<RendererContext> Create();
	};

}

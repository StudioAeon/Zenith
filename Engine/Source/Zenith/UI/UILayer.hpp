#pragma once

#include "Zenith/Core/Layer.hpp"

namespace Zenith {

	class UILayer : public Layer
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;

		void AllowInputEvents(bool allowEvents);

		static std::shared_ptr<UILayer> Create();
	};

}
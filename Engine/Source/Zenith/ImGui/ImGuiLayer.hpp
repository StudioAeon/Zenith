#pragma once

#include "Zenith/Core/Layer.hpp"

namespace Zenith {

	class ImGuiLayer : public Layer
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;

		void AllowInputEvents(bool allowEvents);

		static std::shared_ptr<ImGuiLayer> Create();
	};

}
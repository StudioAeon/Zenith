#pragma once

#include "Zenith/Events/Event.hpp"
#include "TimeStep.hpp"

namespace Zenith {

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnImGuiRender() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual bool OnEvent(Event& event) { return false; }

		inline const std::string& GetName() const { return m_DebugName; }

		bool IsEnabled() const { return m_Enabled; }
		void SetEnabled(bool enabled) { m_Enabled = enabled; }
	protected:
		std::string m_DebugName;
		bool m_Enabled = true;
	};

}

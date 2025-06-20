#pragma once

#include "Zenith/Core/Layer.hpp"

namespace Zenith {

	class ApplicationContext;

	class ImGuiLayer : public Layer
	{
	public:
		static std::shared_ptr<ImGuiLayer> Create(ApplicationContext& context);

		virtual ~ImGuiLayer() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;

		virtual void OnAttach() override = 0;
		virtual void OnDetach() override = 0;

		virtual void OnUpdate(Timestep ts) override {}
		virtual void OnImGuiRender() override {}
		virtual bool OnEvent(Event& event) override { return false; }

	protected:
		explicit ImGuiLayer(ApplicationContext& context);

		ApplicationContext& GetContext() { return m_Context; }
		const ApplicationContext& GetContext() const { return m_Context; }

	private:
		ApplicationContext& m_Context;
	};

}
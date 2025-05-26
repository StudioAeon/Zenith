#pragma once

#include "Base.hpp"
#include "Assert.hpp"
#include "Layer.hpp"

#include <vector>
#include <memory>

namespace Zenith {

	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void Clear();

		void PushLayer(const std::shared_ptr<Layer>& layer);
		void PushOverlay(const std::shared_ptr<Layer>& overlay);
		void PopLayer(const std::shared_ptr<Layer>& layer);
		void PopOverlay(const std::shared_ptr<Layer>& overlay);

		std::shared_ptr<Layer>& operator[](size_t index)
		{
			ZN_CORE_ASSERT(index < m_Layers.size(), "Layer index out of bounds");
			return m_Layers[index];
		}

		const std::shared_ptr<Layer>& operator[](size_t index) const
		{
			ZN_CORE_ASSERT(index < m_Layers.size(), "Layer index out of bounds");
			return m_Layers[index];
		}

		size_t Size() const { return m_Layers.size(); }

		auto begin() { return m_Layers.begin(); }
		auto end() { return m_Layers.end(); }
		auto begin() const { return m_Layers.begin(); }
		auto end() const { return m_Layers.end(); }

	private:
		std::vector<std::shared_ptr<Layer>> m_Layers;
		uint32_t m_LayerInsertIndex = 0;
	};

}

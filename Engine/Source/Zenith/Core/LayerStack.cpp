#include "znpch.hpp"
#include "LayerStack.hpp"

namespace Zenith {

	LayerStack::~LayerStack()
	{}

	void LayerStack::Clear()
	{
		m_Layers.clear();
		m_LayerInsertIndex = 0;
	}

	void LayerStack::PushLayer(const std::shared_ptr<Layer>& layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
		m_LayerInsertIndex++;
	}

	void LayerStack::PushOverlay(const std::shared_ptr<Layer>& overlay)
	{
		m_Layers.push_back(overlay);
	}

	void LayerStack::PopLayer(const std::shared_ptr<Layer>& layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
		if (it != m_Layers.begin() + m_LayerInsertIndex)
		{
			(*it)->OnDetach();
			m_Layers.erase(it);
			m_LayerInsertIndex--;
		}
	}

	void LayerStack::PopOverlay(const std::shared_ptr<Layer>& overlay)
	{
		auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			(*it)->OnDetach();
			m_Layers.erase(it);
		}
	}

}
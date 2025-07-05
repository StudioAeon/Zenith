#pragma once

#include "Zenith/Core/Ref.hpp"
#include "Pipeline.hpp"
#include "vulkan/vulkan.h"

namespace Zenith {
	class Application;

	class RenderCommandBuffer : public RefCounted
	{
	public:
		RenderCommandBuffer(uint32_t count = 0, std::string debugName = "");
		RenderCommandBuffer(std::string debugName, bool swapchain);
		~RenderCommandBuffer();

		void Begin();
		void End();
		void Submit();

		float GetExecutionGPUTime(uint32_t frameIndex, uint32_t queryIndex = 0) const
		{
			if (queryIndex == UINT32_MAX || queryIndex / 2 >= m_TimestampNextAvailableQuery / 2)
				return 0.0f;

			return m_ExecutionGPUTimes[frameIndex][queryIndex / 2];
		}

		const PipelineStatistics& GetPipelineStatistics(uint32_t frameIndex) const { return m_PipelineStatisticsQueryResults[frameIndex]; }

		uint32_t BeginTimestampQuery();
		void EndTimestampQuery(uint32_t queryID);

		VkCommandBuffer GetActiveCommandBuffer() const { return m_ActiveCommandBuffer; }

		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
		{
			ZN_CORE_ASSERT(frameIndex < m_CommandBuffers.size());
			return m_CommandBuffers[frameIndex];
		}

		static Ref<RenderCommandBuffer> Create(uint32_t count = 0, const std::string& debugName = "")
		{
			return Ref<RenderCommandBuffer>::Create(count, debugName);
		}

		static Ref<RenderCommandBuffer> CreateFromSwapChain(const std::string& debugName = "")
		{
			return Ref<RenderCommandBuffer>::Create(debugName, true);
		}

	private:
		std::string m_DebugName;
		VkCommandPool m_CommandPool = nullptr;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		VkCommandBuffer m_ActiveCommandBuffer = nullptr;
		std::vector<VkFence> m_WaitFences;

		bool m_OwnedBySwapChain = false;

		Application* m_Application = nullptr;

		uint32_t m_TimestampQueryCount = 0;
		uint32_t m_TimestampNextAvailableQuery = 2;
		std::vector<VkQueryPool> m_TimestampQueryPools;
		std::vector<VkQueryPool> m_PipelineStatisticsQueryPools;
		std::vector<std::vector<uint64_t>> m_TimestampQueryResults;
		std::vector<std::vector<float>> m_ExecutionGPUTimes;

		uint32_t m_PipelineQueryCount = 0;
		std::vector<PipelineStatistics> m_PipelineStatisticsQueryResults;
	};

}
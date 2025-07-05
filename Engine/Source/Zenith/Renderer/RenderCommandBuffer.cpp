#include "znpch.hpp"
#include "RenderCommandBuffer.hpp"

#include "Zenith/Core/Application.hpp"
#include "Zenith/Renderer/Renderer.hpp"
#include "API/Vulkan/VulkanContext.hpp"
#include "API/Vulkan/VulkanSwapChain.hpp"
#include "API/Vulkan/VulkanDevice.hpp"
#include "API/Vulkan/VulkanAPI.hpp"

namespace Zenith {

	RenderCommandBuffer::RenderCommandBuffer(uint32_t count, std::string debugName)
		: m_DebugName(std::move(debugName))
	{
		m_Application = Renderer::GetApplication();
		if (!m_Application) {
			ZN_CORE_ERROR("RenderCommandBuffer::RenderCommandBuffer - Application is null!");
			return;
		}

		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device->GetVulkanDevice(), &cmdPoolInfo, nullptr, &m_CommandPool));

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = m_CommandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = count == 0 ? framesInFlight : count;

		m_CommandBuffers.resize(cmdBufAllocateInfo.commandBufferCount);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &cmdBufAllocateInfo, m_CommandBuffers.data()));

		m_WaitFences.resize(framesInFlight);
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (auto& fence : m_WaitFences)
			VK_CHECK_RESULT(vkCreateFence(device->GetVulkanDevice(), &fenceCreateInfo, nullptr, &fence));

		// Timestamp queries
		uint32_t commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
		m_TimestampQueryCount = 32 * 2; // 32 timestamp pairs
		VkQueryPoolCreateInfo queryPoolCreateInfo = {};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = m_TimestampQueryCount;

		m_TimestampQueryPools.resize(commandBufferCount);
		for (auto& timestampQueryPool : m_TimestampQueryPools)
			VK_CHECK_RESULT(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &timestampQueryPool));

		m_TimestampQueryResults.resize(commandBufferCount);
		for (auto& timestampQueryResults : m_TimestampQueryResults)
			timestampQueryResults.resize(m_TimestampQueryCount);

		m_ExecutionGPUTimes.resize(commandBufferCount);
		for (auto& executionGPUTimes : m_ExecutionGPUTimes)
			executionGPUTimes.resize(m_TimestampQueryCount / 2);

		// Pipeline statistics queries
		m_PipelineQueryCount = 7;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		queryPoolCreateInfo.queryCount = m_PipelineQueryCount;
		queryPoolCreateInfo.pipelineStatistics =
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

		m_PipelineStatisticsQueryPools.resize(commandBufferCount);
		for (auto& pipelineStatisticsQueryPools : m_PipelineStatisticsQueryPools)
			VK_CHECK_RESULT(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPools));

		m_PipelineStatisticsQueryResults.resize(commandBufferCount);
	}

	RenderCommandBuffer::RenderCommandBuffer(std::string debugName, bool swapchain)
		: m_DebugName(std::move(debugName)), m_OwnedBySwapChain(true)
	{
		m_Application = Renderer::GetApplication();
		if (!m_Application) {
			ZN_CORE_ERROR("RenderCommandBuffer::RenderCommandBuffer - Application is null!");
			return;
		}

		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		// Setup query pools for swapchain command buffers
		m_TimestampQueryCount = 32 * 2;
		VkQueryPoolCreateInfo queryPoolCreateInfo = {};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = m_TimestampQueryCount;

		m_TimestampQueryPools.resize(framesInFlight);
		for (auto& timestampQueryPool : m_TimestampQueryPools)
			VK_CHECK_RESULT(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &timestampQueryPool));

		m_TimestampQueryResults.resize(framesInFlight);
		for (auto& timestampQueryResults : m_TimestampQueryResults)
			timestampQueryResults.resize(m_TimestampQueryCount);

		m_ExecutionGPUTimes.resize(framesInFlight);
		for (auto& executionGPUTimes : m_ExecutionGPUTimes)
			executionGPUTimes.resize(m_TimestampQueryCount / 2);

		// Pipeline statistics queries
		m_PipelineQueryCount = 7;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		queryPoolCreateInfo.queryCount = m_PipelineQueryCount;
		queryPoolCreateInfo.pipelineStatistics =
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

		m_PipelineStatisticsQueryPools.resize(framesInFlight);
		for (auto& pipelineStatisticsQueryPools : m_PipelineStatisticsQueryPools)
			VK_CHECK_RESULT(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPools));

		m_PipelineStatisticsQueryResults.resize(framesInFlight);
	}

	RenderCommandBuffer::~RenderCommandBuffer()
	{
		if (m_OwnedBySwapChain)
			return;

		VkCommandPool commandPool = m_CommandPool;
		Renderer::SubmitResourceFree([commandPool]()
			{
				auto device = VulkanContext::GetCurrentDevice();
				vkDestroyCommandPool(device->GetVulkanDevice(), commandPool, nullptr);
			});
	}

	void RenderCommandBuffer::Begin()
	{
		m_TimestampNextAvailableQuery = 2;

		Ref<RenderCommandBuffer> instance = this;
		Application* app = m_Application;

		Renderer::Submit([instance, app]() mutable
		{
				uint32_t commandBufferIndex = Renderer::RT_GetCurrentFrameIndex();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cmdBufInfo.pNext = nullptr;

			VkCommandBuffer commandBuffer = nullptr;
			if (instance->m_OwnedBySwapChain)
			{
				VulkanSwapChain& swapChain = app->GetWindow().GetSwapChain();
				commandBuffer = swapChain.GetDrawCommandBuffer(commandBufferIndex);
			}
			else
			{
				commandBufferIndex %= instance->m_CommandBuffers.size();
				commandBuffer = instance->m_CommandBuffers[commandBufferIndex];
			}
			instance->m_ActiveCommandBuffer = commandBuffer;
			VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

			// Timestamp query
			vkCmdResetQueryPool(commandBuffer, instance->m_TimestampQueryPools[commandBufferIndex], 0, instance->m_TimestampQueryCount);
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->m_TimestampQueryPools[commandBufferIndex], 0);

			// Pipeline stats query
			vkCmdResetQueryPool(commandBuffer, instance->m_PipelineStatisticsQueryPools[commandBufferIndex], 0, instance->m_PipelineQueryCount);
			vkCmdBeginQuery(commandBuffer, instance->m_PipelineStatisticsQueryPools[commandBufferIndex], 0, 0);
		});
	}

	void RenderCommandBuffer::End()
	{
		Ref<RenderCommandBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			uint32_t commandBufferIndex = Renderer::RT_GetCurrentFrameIndex();
			if (!instance->m_OwnedBySwapChain)
				commandBufferIndex %= instance->m_CommandBuffers.size();

			VkCommandBuffer commandBuffer = instance->m_ActiveCommandBuffer;
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->m_TimestampQueryPools[commandBufferIndex], 1);
			vkCmdEndQuery(commandBuffer, instance->m_PipelineStatisticsQueryPools[commandBufferIndex], 0);
			VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

			instance->m_ActiveCommandBuffer = nullptr;
		});
	}

	void RenderCommandBuffer::Submit()
	{
		if (m_OwnedBySwapChain)
			return;

		Ref<RenderCommandBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			uint32_t commandBufferIndex = Renderer::RT_GetCurrentFrameIndex() % instance->m_CommandBuffers.size();

			VkFence waitFence = instance->m_WaitFences[commandBufferIndex];

			auto device = VulkanContext::GetCurrentDevice();
			vkWaitForFences(device->GetVulkanDevice(), 1, &waitFence, VK_TRUE, UINT64_MAX);
			vkResetFences(device->GetVulkanDevice(), 1, &waitFence);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &instance->m_CommandBuffers[commandBufferIndex];

			VK_CHECK_RESULT(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, waitFence));

			// Get query results
			auto queryResultFlags = VK_QUERY_RESULT_64_BIT;
			vkGetQueryPoolResults(device->GetVulkanDevice(), instance->m_TimestampQueryPools[commandBufferIndex], 0, instance->m_TimestampQueryCount,
				instance->m_TimestampQueryResults[commandBufferIndex].size() * sizeof(uint64_t), instance->m_TimestampQueryResults[commandBufferIndex].data(), sizeof(uint64_t), queryResultFlags);

			for (uint32_t i = 0; i < instance->m_TimestampQueryCount; i += 2)
			{
				uint64_t startTime = instance->m_TimestampQueryResults[commandBufferIndex][i];
				uint64_t endTime = instance->m_TimestampQueryResults[commandBufferIndex][i + 1];
				float nsTime = endTime > startTime ? (endTime - startTime) * device->GetPhysicalDevice()->GetLimits().timestampPeriod : 0.0f;
				instance->m_ExecutionGPUTimes[commandBufferIndex][i / 2] = nsTime * 0.000001f; // Time in ms
			}

			// Retrieve pipeline stats results
			vkGetQueryPoolResults(device->GetVulkanDevice(), instance->m_PipelineStatisticsQueryPools[commandBufferIndex], 0, 1,
				sizeof(PipelineStatistics), &instance->m_PipelineStatisticsQueryResults[commandBufferIndex], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
		});
	}

	uint32_t RenderCommandBuffer::BeginTimestampQuery()
	{
		uint32_t queryIndex = m_TimestampNextAvailableQuery;
		m_TimestampNextAvailableQuery += 2;
		Ref<RenderCommandBuffer> instance = this;
		Renderer::Submit([instance, queryIndex]()
		{
			uint32_t commandBufferIndex = Renderer::RT_GetCurrentFrameIndex() % instance->m_CommandBuffers.size();
			VkCommandBuffer commandBuffer = instance->m_CommandBuffers[commandBufferIndex];
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->m_TimestampQueryPools[commandBufferIndex], queryIndex);
		});
		return queryIndex;
	}

	void RenderCommandBuffer::EndTimestampQuery(uint32_t queryID)
	{
		Ref<RenderCommandBuffer> instance = this;
		Renderer::Submit([instance, queryID]()
		{
			uint32_t commandBufferIndex = Renderer::RT_GetCurrentFrameIndex() % instance->m_CommandBuffers.size();
			VkCommandBuffer commandBuffer = instance->m_CommandBuffers[commandBufferIndex];
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->m_TimestampQueryPools[commandBufferIndex], queryID + 1);
		});
	}

}
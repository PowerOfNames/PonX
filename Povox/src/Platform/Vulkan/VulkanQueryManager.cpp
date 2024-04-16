#include "pxpch.h"
#include "Platform/Vulkan/VulkanQueryManager.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"

#include "Povox/Core/Log.h"

namespace Povox {


	VulkanQueryManager::VulkanQueryManager()
	{

	}

	void VulkanQueryManager::Shutdown()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		m_PipelineStatisticsQueries.clear();
		m_TimestampQueries.clear();

		for (auto& [name, poolInfo] : m_PipelineStatisticsQueryPools)
		{
			for (uint32_t i = 0; i < poolInfo.Pools.size(); i++)
			{
				vkDestroyQueryPool(device, poolInfo.Pools[i], nullptr);
			}
			poolInfo.Pools.clear();
		}
		m_PipelineStatisticsQueryPools.clear();

		for (uint32_t i = 0; i < m_TimestampQueryPools.Pools.size(); i++)
		{
			vkDestroyQueryPool(device, m_TimestampQueryPools.Pools[i], nullptr);
		}
		m_TimestampQueryPools.Pools.clear();
	}

	void VulkanQueryManager::CreateTimestampQueryPools(uint32_t framesInFlight, uint32_t poolQueryCount/* = 20*/)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
						
		VkQueryPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.queryCount = poolQueryCount;
		info.queryType = VK_QUERY_TYPE_TIMESTAMP;
		info.pipelineStatistics = 0;

		m_TimestampQueryPools.Pools.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			PX_CORE_VK_ASSERT(vkCreateQueryPool(device, &info, nullptr, &m_TimestampQueryPools.Pools[i]), VK_SUCCESS, "Failed to create TimeStamp pool!");
			vkResetQueryPool(device, m_TimestampQueryPools.Pools[i], 0, m_TimestampQueries.size() * 2);
			m_TimestampQueryPools.QueriesPerPool = poolQueryCount;		
		}
	}
	
	void VulkanQueryManager::RecreateTimestampPools(uint32_t newPoolQueryCount)
	{
		m_TimestampQueryPools.QueriesPerPool = newPoolQueryCount;
	}
		

	void VulkanQueryManager::AddTimestampQuery(const std::string& name, uint32_t count)
	{
		if (m_TimestampQueries.find(name) != m_TimestampQueries.end())
		{
			PX_CORE_INFO("VulkanQueryManager::AddTimestampQuery: Timestamp {} already registered!", name);
			return;
		}

		if (m_TimestampQueryPools.Pools.size() < 1)
		{
			PX_CORE_ERROR("VulkanQueryManager::AddTimestampQuery: PipelineQueries are uninitielized!");
			return;
		}

		
		if (m_TimestampQueryPools.NextFreeIndex + count > m_TimestampQueryPools.QueriesPerPool)
			RecreateTimestampPools(m_TimestampQueryPools.QueriesPerPool + count + (uint32_t)(m_TimestampQueryPools.QueriesPerPool * 0.5));

		m_TimestampQueries[name] = QueryData{ "TimestampPool", m_TimestampQueryPools.NextFreeIndex, count};
		m_TimestampQueryPools.NextFreeIndex += count;
	}

	void VulkanQueryManager::RecordTimestamp(const std::string& name, uint32_t frameIdx, VkCommandBuffer cmd)
	{
		if (m_TimestampQueries.find(name) == m_TimestampQueries.end())
		{
			PX_CORE_ERROR("VulkanQueryManager::RecordTimestamp: TimestampQuery for {} not registered!", name);
			return;
		}

		if (frameIdx > m_TimestampQueryPools.Pools.size() - 1)
		{
			PX_CORE_ERROR("VulkanQueryManager::RecordTimestamp: FrameIdx {} out of scope {}!", frameIdx, m_TimestampQueryPools.Pools.size() - 1);
			return;
		}

		QueryData& qd = m_TimestampQueries.at(name);
		if (qd.CurrentIncrement +1 < qd.Count)
		{			
			vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, m_TimestampQueryPools.Pools[frameIdx], qd.IndexInPool + qd.CurrentIncrement);
			qd.CurrentIncrement++;
			return;
		}
		PX_CORE_WARN("VulkanQueryManager::RecordTimestamp: TimestampQuery for {} reached maximum timestamps ({}/{})", name, qd.CurrentIncrement +1, qd.Count);
	}

	const std::unordered_map<std::string, std::vector<uint64_t>> VulkanQueryManager::GetTimestampQueryResults(uint32_t frameIdx)
	{
		if (m_TimestampQueryPools.Pools.size() < 1)
			return;

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		PoolInfo& poolInfo = m_TimestampQueryPools;
		std::vector<uint64_t> poolResults;
		poolResults.resize(poolInfo.QueriesPerPool);
		vkGetQueryPoolResults(
			device,
			poolInfo.Pools[frameIdx],
			0,
			poolInfo.QueriesPerPool,
			poolInfo.QueriesPerPool * sizeof(uint64_t),
			poolResults.data(),
			sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT);

		std::unordered_map<std::string, std::vector<uint64_t>> results;
		size_t currendIdx = 0;
		for (auto& [queryName, queryData] : m_PipelineStatisticsQueries)
		{
			results[queryName].emplace_back(poolResults.begin() + queryData.IndexInPool, poolResults.begin() + queryData.IndexInPool + queryData.CurrentIncrement);
			queryData.CurrentIncrement = 0;
		}
		return results;
	}
	
	void VulkanQueryManager::ResetTimestampQueryPool(uint32_t frameIdx)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		vkResetQueryPool(device, m_TimestampQueryPools.Pools[frameIdx], 0, m_TimestampQueryPools.QueriesPerPool);
	}

	
	// Creating a normal pipeline query (graphics) will automatically also create one query of count 6 with the same name as its pool 
	void VulkanQueryManager::CreatePipelineStatisticsQueryPool(const std::string& name, uint32_t framesInFlight, VkPipelineBindPoint pipelineBindPoint)
	{		
		PX_CORE_INFO("Starting PipelineStatisticsPool creation...");
	
		if (m_PipelineStatisticsQueryPools.find(name) != m_PipelineStatisticsQueryPools.end())
		{
			PX_CORE_INFO("VulkanQueryManager::CreatePipelineStatisticsQueryPool: Pool {} already registered!", name);
			return;
		}

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		
		PoolInfo& newPool = m_PipelineStatisticsQueryPools[name];
		newPool.Pools.resize(framesInFlight);

		VkQueryPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
		{
			info.queryCount = 6;
			info.pipelineStatistics =
				VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
				VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
				VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;

			newPool.QueriesPerPool = 6;
			newPool.NextFreeIndex = 0;
			AddPipelineStatisticsQuery(name, name, 6);
		}
		else if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE)
		{
			info.queryCount = 1;
			info.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

			newPool.QueriesPerPool = 1;			
		}

		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			PX_CORE_VK_ASSERT(vkCreateQueryPool(device, &info, nullptr, &newPool.Pools[i]), VK_SUCCESS, "Failed to create Pipeline stats pool!");
			vkResetQueryPool(device, newPool.Pools[i], 0, 1);
		}
		
		PX_CORE_INFO("Completed PipelineStaticsticsPool creation.");
	}
	
	void VulkanQueryManager::RecreatePipelineQueryPools(uint32_t newPoolQuerySize)
	{

	}

	void VulkanQueryManager::AddPipelineStatisticsQuery(const std::string& name, const std::string& poolName, uint32_t count/* = 1*/)
	{
		if (m_PipelineStatisticsQueryPools.find(name) == m_PipelineStatisticsQueryPools.end())
		{
			PX_CORE_INFO("VulkanQueryManager::CreatePipelineStatisticsQueryPool: Pool {} not registered!", name);
			return;
		}
		if (m_PipelineStatisticsQueries.find(name) != m_PipelineStatisticsQueries.end())
		{
			PX_CORE_INFO("VulkanQueryManager::AddComputeStatisticsQuery: Timestamp {} already registered!", name);
			return;
		}
		if (m_PipelineStatisticsQueryPools.at(poolName).Pools.size() < 1)
		{
			PX_CORE_ERROR("VulkanQueryManager::AddComputeStatisticsQuery: PipelineQueries are uninitielized!");
			return;
		}

		PoolInfo& poolInfo = m_PipelineStatisticsQueryPools.at(poolName);
		if (poolInfo.NextFreeIndex + count > poolInfo.QueriesPerPool)
			RecreatePipelineQueryPools(poolInfo.QueriesPerPool + count + (uint32_t)(poolInfo.QueriesPerPool * 0.5));

		m_PipelineStatisticsQueries[name] = QueryData{ poolName, poolInfo.NextFreeIndex, count };
		poolInfo.NextFreeIndex += count;		
	}

	void VulkanQueryManager::BeginPipelineQuery(const std::string& name, VkCommandBuffer cmd, uint32_t frameIdx)
	{		
		if (m_PipelineStatisticsQueries.find(name) == m_PipelineStatisticsQueries.end())
		{
			PX_CORE_INFO("VulkanQueryManager::CreatePipelineStatisticsQueryPool: Pool {} not registered!", name);
			return;
		}
		QueryData& queryData = m_PipelineStatisticsQueries.at(name);
		vkCmdBeginQuery(cmd, m_PipelineStatisticsQueryPools.at(queryData.PoolName).Pools[frameIdx], queryData.IndexInPool + queryData.CurrentIncrement, 0);
	}

	void VulkanQueryManager::EndPipelineQuery(const std::string& name, VkCommandBuffer cmd, uint32_t frameIdx)
	{
		QueryData& queryData = m_PipelineStatisticsQueries.at(name);
		vkCmdEndQuery(cmd, m_PipelineStatisticsQueryPools.at(queryData.PoolName).Pools[frameIdx], queryData.IndexInPool + queryData.CurrentIncrement);
	}

	const std::unordered_map<std::string, std::vector<uint64_t>> VulkanQueryManager::GetPipelineQueryResults(const std::string& poolName, uint32_t frameIdx)
	{		
		if (m_PipelineStatisticsQueryPools.find(poolName) == m_PipelineStatisticsQueryPools.end())
		{
			PX_CORE_INFO("VulkanQueryManager::CreatePipelineStatisticsQueryPool: Pool {} not registered!", poolName);
			return;
		}

		if (!m_PipelineStatisticsQueryPools.at(poolName).Pools[frameIdx])
			return;

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		PoolInfo& poolInfo = m_PipelineStatisticsQueryPools.at(poolName);
		std::vector<uint64_t> poolResults;
		poolResults.resize(poolInfo.QueriesPerPool);
		vkGetQueryPoolResults(
			device, 
			poolInfo.Pools[frameIdx],
			0, 
			poolInfo.QueriesPerPool,
			poolInfo.QueriesPerPool * sizeof(uint64_t),
			poolResults.data(),
			sizeof(uint64_t), 
			VK_QUERY_RESULT_64_BIT);

		std::unordered_map<std::string, std::vector<uint64_t>> results;
		size_t currendIdx = 0;
		for (auto& [queryName, queryData] : m_PipelineStatisticsQueries)
		{
			results[queryName].emplace_back(poolResults.begin() + queryData.IndexInPool, poolResults.begin() + queryData.IndexInPool + queryData.CurrentIncrement);
			queryData.CurrentIncrement = 0;
		}
		return results;
	}

	void VulkanQueryManager::ResetPipelineQueryPools(uint32_t frameIdx)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		for (auto& [name, pool] : m_PipelineStatisticsQueryPools)
		{
			vkResetQueryPool(device, pool.Pools[frameIdx], 0, pool.QueriesPerPool);
		}
	}

}

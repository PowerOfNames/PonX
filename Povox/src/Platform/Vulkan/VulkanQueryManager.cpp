#include "pxpch.h"
#include "Platform/Vulkan/VulkanQueryManager.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"

#include "Povox/Core/Log.h"

namespace Povox {

	namespace VulkanUtils
	{
		static const char* QueryPipelineStatisticsFlagToString(VkQueryPipelineStatisticFlagBits flag)
		{				
			switch (flag)
			{
				case VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT: return "Input assembly vertex count (indices): ";
				case VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT: return "Input assembly primitives count: ";
				case VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT: return "Vertex shader invocations: ";
				case VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT: return "Clipping stage invocations:  ";
				case VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT: return "Clipping stage - primitives reached: ";
				case VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT: return "Fragment shader invocations: ";
				default: 
				{
					PX_CORE_WARN("VulkanUtils::QueryPipelineStatisticsFlagToString: Query Pipeline statictics flag not covered!");
					return "Missing Pipeline Stage Flag!";
				}
			}
		}
	}

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

	//PoolQueryCount must be even
	void VulkanQueryManager::CreateTimestampQueryPools(uint32_t framesInFlight, uint32_t poolQueryCount/* = 20*/)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		
		if (poolQueryCount % 2 == 1)
			poolQueryCount++;

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

#ifdef PX_DEBUG
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_QUERY_POOL;
			nameInfo.objectHandle = (uint64_t)m_TimestampQueryPools.Pools[i];
			nameInfo.pObjectName = "TimestampQueryPool";
			NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG

			vkResetQueryPool(device, m_TimestampQueryPools.Pools[i], 0, poolQueryCount);
			m_TimestampQueryPools.QueriesPerPool = poolQueryCount;
			// I only want to cache final results, not start and end -> QueryData.IndexIntoPool points to start +1 to end, but indexIntoPool is used to refer to value inside LastAvailableResult
			m_TimestampQueryPools.LastAvailableResults.resize(poolQueryCount / 2);
			m_TimestampQueryPools.AllResultsAvailable[i] = true;
		}
	}
	
	void VulkanQueryManager::RecreateTimestampPools(uint32_t newPoolQueryCount)
	{
		PX_CORE_WARN("VulkanQueryManager::RecreateTimestampPools: Not implemented!");
		return;
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

		QueryData data{};
		data.PoolName = "TimestampQueryPool";
		data.Count = count;
		data.IndexInPool = m_TimestampQueryPools.NextFreeIndex;
		data.QueryNumber = m_TimestampQueryPools.TotalQueries;
		m_TimestampQueries[name] = std::move(data);

		m_TimestampQueryPools.NextFreeIndex += count;
		m_TimestampQueryPools.TotalQueries++;
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
		if (qd.CurrentIncrement >= qd.Count)
		{			
			PX_CORE_WARN("VulkanQueryManager::RecordTimestamp: TimestampQuery for {} reached maximum timestamps ({}/{})", name, qd.CurrentIncrement, qd.Count);
			return;
		}
		if (qd.ResultAvailable)
		{
			VkPipelineStageFlagBits2 stage = qd.CurrentIncrement ? VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			vkCmdWriteTimestamp2(cmd, stage, m_TimestampQueryPools.Pools[frameIdx], qd.IndexInPool + qd.CurrentIncrement);
			qd.CurrentIncrement++;
		}
	}

	const std::unordered_map<std::string, uint64_t> VulkanQueryManager::GetTimestampQueryResults(uint32_t frameIdx)
	{
		std::unordered_map<std::string, uint64_t> results;
		if (m_TimestampQueryPools.Pools.size() < 1)
			return results;

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		PoolInfo& poolInfo = m_TimestampQueryPools;
		std::vector<uint64_t> frameResults;
		frameResults.resize(poolInfo.QueriesPerPool * 2);
		vkGetQueryPoolResults(
			device,
			poolInfo.Pools[frameIdx],
			0,
			poolInfo.QueriesPerPool,
			frameResults.size() * sizeof(uint64_t),
			frameResults.data(),
			sizeof(uint64_t) * 2,
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
		);

		float timestampPeriod = VulkanContext::GetDevice()->GetLimits().TimestampPeriod;
		for (auto& [queryName, queryData] : m_TimestampQueries)
		{
			poolInfo.AllResultsAvailable[frameIdx] = true;
			uint32_t queryResultIdx = queryData.IndexInPool * 2;
			if (frameResults[queryResultIdx + 1] == 0)
			{
				poolInfo.AllResultsAvailable[frameIdx] = false;
				queryData.ResultAvailable = false;
			}
			else
			{
				queryData.ResultAvailable = true;
				poolInfo.LastAvailableResults[queryData.QueryNumber] = frameResults[queryResultIdx + 2] - frameResults[queryResultIdx];
			}
			results[queryName] = poolInfo.LastAvailableResults[queryData.QueryNumber];
		}
		return results;
	}
	
	void VulkanQueryManager::ResetTimestampQueryPool(uint32_t frameIdx)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		
		if (m_TimestampQueryPools.AllResultsAvailable[frameIdx])
		{
			vkResetQueryPool(device, m_TimestampQueryPools.Pools[frameIdx], 0, m_TimestampQueryPools.QueriesPerPool);
			for (auto& [queryName, data] : m_TimestampQueries)
			{
				data.CurrentIncrement = 0;
			}
			return;
		}

		for (auto& [queryName, data] : m_TimestampQueries)
		{
			if (data.ResultAvailable)
			{
				vkResetQueryPool(device, m_TimestampQueryPools.Pools[frameIdx], data.IndexInPool, data.Count);
				data.CurrentIncrement = 0;
			}
		}
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
			info.queryCount = 1;
			info.pipelineStatistics =
				VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
				VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
				VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;

			newPool.QueriesPerPool = 1;
			newPool.QueryStatCount = 6;
			AddPipelineStatisticsQuery(VulkanUtils::QueryPipelineStatisticsFlagToString(VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT), name);
			AddPipelineStatisticsQuery(VulkanUtils::QueryPipelineStatisticsFlagToString(VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT), name);
			AddPipelineStatisticsQuery(VulkanUtils::QueryPipelineStatisticsFlagToString(VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT), name);
			AddPipelineStatisticsQuery(VulkanUtils::QueryPipelineStatisticsFlagToString(VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT), name);
			AddPipelineStatisticsQuery(VulkanUtils::QueryPipelineStatisticsFlagToString(VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT), name);
			AddPipelineStatisticsQuery(VulkanUtils::QueryPipelineStatisticsFlagToString(VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT), name);
		}
		else if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE)
		{
			info.queryCount = 1;
			info.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

			newPool.QueriesPerPool = 1;
			newPool.QueryStatCount = 1;
		}

		newPool.LastAvailableResults.resize(newPool.QueriesPerPool * newPool.QueryStatCount);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			PX_CORE_VK_ASSERT(vkCreateQueryPool(device, &info, nullptr, &newPool.Pools[i]), VK_SUCCESS, "Failed to create Pipeline stats pool!");

#ifdef PX_DEBUG
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_QUERY_POOL;
			nameInfo.objectHandle = (uint64_t)newPool.Pools[i];
			nameInfo.pObjectName = name.c_str();
			NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG

			vkResetQueryPool(device, newPool.Pools[i], 0, newPool.QueriesPerPool);
			newPool.AllResultsAvailable[i] = true;
		}
		
		PX_CORE_INFO("Completed PipelineStaticsticsPool creation.");
	}
	
	void VulkanQueryManager::RecreatePipelineQueryPool(const std::string& poolName, uint32_t newPoolQuerySize)
	{
		PX_CORE_WARN("VulkanQueryManager::RecreatePipelineQueryPool: Not implemented!");
		return;
		PoolInfo& poolInfo = m_PipelineStatisticsQueryPools.at(poolName);
		poolInfo.QueriesPerPool = newPoolQuerySize;
	}

	void VulkanQueryManager::AddPipelineStatisticsQuery(const std::string& name, const std::string& poolName)
	{
		if (m_PipelineStatisticsQueryPools.find(poolName) == m_PipelineStatisticsQueryPools.end())
		{
			PX_CORE_INFO("VulkanQueryManager::AddPipelineStatisticsQuery: Pool {} not registered!", name);
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
		
		if (poolInfo.NextFreeIndex + 1 > poolInfo.QueriesPerPool * poolInfo.QueryStatCount)
			RecreatePipelineQueryPool(poolName, poolInfo.QueriesPerPool + 1 + (uint32_t)(poolInfo.QueriesPerPool * 0.5));

		QueryData data{};
		data.PoolName = poolName;
		data.IndexInPool = poolInfo.NextFreeIndex;
		data.QueryNumber = poolInfo.TotalQueries;

		m_PipelineStatisticsQueries[name] = std::move(data);
		poolInfo.NextFreeIndex++;	
	}

	void VulkanQueryManager::BeginPipelineQuery(const std::string& name, VkCommandBuffer cmd, uint32_t frameIdx)
	{		
		if (m_PipelineStatisticsQueryPools.find(name) != m_PipelineStatisticsQueryPools.end())
		{
			PoolInfo& pool = m_PipelineStatisticsQueryPools.at(name);
			if (pool.AllResultsAvailable[frameIdx])
				vkCmdBeginQuery(cmd, pool.Pools[frameIdx], 0, 0);
			return;
		}
		if (m_PipelineStatisticsQueries.find(name) == m_PipelineStatisticsQueries.end())
		{
			PX_CORE_INFO("VulkanQueryManager::BeginPipelineQuery: Query {} not registered!", name);
			return;
		}
		QueryData& queryData = m_PipelineStatisticsQueries.at(name);
		if(queryData.ResultAvailable)
			vkCmdBeginQuery(cmd, m_PipelineStatisticsQueryPools.at(queryData.PoolName).Pools[frameIdx], queryData.IndexInPool + queryData.CurrentIncrement, 0);
	}

	void VulkanQueryManager::EndPipelineQuery(const std::string& name, VkCommandBuffer cmd, uint32_t frameIdx)
	{
		if (m_PipelineStatisticsQueryPools.find(name) != m_PipelineStatisticsQueryPools.end())
		{
			PoolInfo& pool = m_PipelineStatisticsQueryPools.at(name);
			if (pool.AllResultsAvailable[frameIdx])
				vkCmdEndQuery(cmd, m_PipelineStatisticsQueryPools.at(name).Pools[frameIdx], 0);
			return;
		}
		if (m_PipelineStatisticsQueries.find(name) == m_PipelineStatisticsQueries.end())
		{
			PX_CORE_INFO("VulkanQueryManager::EndPipelineQuery: Pool {} not registered!", name);
			return;
		}
		QueryData& queryData = m_PipelineStatisticsQueries.at(name);
		if (queryData.ResultAvailable)
			vkCmdEndQuery(cmd, m_PipelineStatisticsQueryPools.at(queryData.PoolName).Pools[frameIdx], queryData.IndexInPool + queryData.CurrentIncrement);
	}

	const std::unordered_map<std::string, uint64_t> VulkanQueryManager::GetPipelineQueryResults(const std::string& poolName, uint32_t frameIdx)
	{		
		std::unordered_map<std::string, uint64_t> results;
		if (m_PipelineStatisticsQueryPools.find(poolName) == m_PipelineStatisticsQueryPools.end())
		{
			PX_CORE_INFO("VulkanQueryManager::GetPipelineQueryResults: Pool {} not registered!", poolName);
			return results;
		}

		if (!m_PipelineStatisticsQueryPools.at(poolName).Pools[frameIdx])
			return results;

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		PoolInfo& poolInfo = m_PipelineStatisticsQueryPools.at(poolName);
		std::vector<uint64_t> frameResults;
		frameResults.resize(poolInfo.QueriesPerPool * poolInfo.QueryStatCount * 2);
		vkGetQueryPoolResults(
			device, 
			poolInfo.Pools[frameIdx],
			0, 
			poolInfo.QueriesPerPool,
			frameResults.size() * sizeof(uint64_t),
			frameResults.data(),
			sizeof(uint64_t) * 2, 
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
				
		for (auto& [queryName, queryData] : m_PipelineStatisticsQueries)
		{
			if (queryData.PoolName != poolName)
				continue;

			poolInfo.AllResultsAvailable[frameIdx] = true;
			//uint32_t queryResultIdx = queryData.IndexInPool * 2;
			if (frameResults[poolInfo.QueriesPerPool + queryData.IndexInPool] == 0)
			{
				poolInfo.AllResultsAvailable[frameIdx] = false;
				queryData.ResultAvailable = false;
			}
			else
			{
				queryData.ResultAvailable = true;
				poolInfo.LastAvailableResults[queryData.IndexInPool] = frameResults[queryData.IndexInPool];
			}
			results[queryName] = poolInfo.LastAvailableResults[queryData.IndexInPool];
		}
		return results;
	}

	void VulkanQueryManager::ResetPipelineQueryPools(uint32_t frameIdx)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		for (auto& [name, pool] : m_PipelineStatisticsQueryPools)
		{
			if(pool.AllResultsAvailable[frameIdx])
				vkResetQueryPool(device, pool.Pools[frameIdx], 0, pool.QueriesPerPool);
		}

		for (auto& [name, qd] : m_PipelineStatisticsQueries)
		{
			qd.CurrentIncrement = 0;
		}
	}

}

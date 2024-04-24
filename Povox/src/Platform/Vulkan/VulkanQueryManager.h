#pragma once
#include "Platform/Vulkan/VulkanDevice.h"

#include <vulkan/vulkan.h>

#include <string>

namespace Povox {

	class VulkanQueryManager
	{
	public:
		VulkanQueryManager();
		~VulkanQueryManager() = default;

		void Shutdown();

		void CreateTimestampQueryPools(uint32_t framesInFlight, uint32_t poolQueryCount = 20);
		void AddTimestampQuery(const std::string& name, uint32_t count);
		void RecordTimestamp(const std::string& name, uint32_t frameIdx, VkCommandBuffer cmd);

		const std::unordered_map<std::string, uint64_t> GetTimestampQueryResults(uint32_t frameIdx);
		void ResetTimestampQueryPool(uint32_t frameIdx);


		void CreatePipelineStatisticsQueryPool(const std::string& name, uint32_t framesInFlight, VkPipelineBindPoint pipelineBindPoint);
		void AddPipelineStatisticsQuery(const std::string& name, const std::string& poolName);
		void BeginPipelineQuery(const std::string& queryName, VkCommandBuffer cmd, uint32_t frameIdx);
		void EndPipelineQuery(const std::string& queryName, VkCommandBuffer cmd, uint32_t frameIdx);

		const std::unordered_map<std::string, uint64_t> GetPipelineQueryResults(const std::string& poolName, uint32_t frameIdx);
		void ResetPipelineQueryPools(uint32_t frameIdx);

	private:
		void RecreateTimestampPools(uint32_t newPoolQueryCount);
		void RecreatePipelineQueryPool(const std::string& poolName, uint32_t newPoolQuerySize);

	private:

		struct PoolInfo
		{
			std::vector<VkQueryPool> Pools;

			uint32_t QueryStatCount = 1;
			uint32_t QueriesPerPool = 1;
			uint32_t NextFreeIndex = 0;
			uint32_t TotalQueries = 0;
			
			std::unordered_map<uint32_t, bool> AllResultsAvailable;			
			std::vector<uint64_t> LastAvailableResults;
		};
		std::unordered_map<std::string, PoolInfo> m_PipelineStatisticsQueryPools;	
		PoolInfo m_TimestampQueryPools;



		struct QueryData
		{
			std::string PoolName;
			uint32_t QueryNumber = 0;
			uint32_t IndexInPool = 0;
			//Count always 1 for pipeline statistics -> one QueryData point per pipeline stage. One Pool per stage arrangement.
			uint32_t Count = 1;

			//Range (0, ..., Count-1) -> Used for determining of start or stop
			uint32_t CurrentIncrement = 0;

			bool ResultAvailable = true;
		};
		std::unordered_map<std::string, QueryData> m_TimestampQueries;
		std::unordered_map<std::string, QueryData> m_PipelineStatisticsQueries;
	};

}

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

		const std::unordered_map<std::string, std::vector<uint64_t>> GetTimestampQueryResults(uint32_t frameIdx);
		void ResetTimestampQueryPool(uint32_t frameIdx);


		void CreatePipelineStatisticsQueryPool(const std::string& name, uint32_t framesInFlight, VkPipelineBindPoint pipelineBindPoint);
		void AddPipelineStatisticsQuery(const std::string& name, const std::string& poolName, uint32_t count = 1);
		void BeginPipelineQuery(const std::string& queryName, VkCommandBuffer cmd, uint32_t frameIdx);
		void EndPipelineQuery(const std::string& queryName, VkCommandBuffer cmd, uint32_t frameIdx);

		const std::unordered_map<std::string, std::vector<uint64_t>> GetPipelineQueryResults(const std::string& poolName, uint32_t frameIdx);
		void ResetPipelineQueryPools(uint32_t frameIdx);

	private:
		void RecreateTimestampPools(uint32_t newPoolQueryCount);
		void RecreatePipelineQueryPools(uint32_t newPoolQuerySize);

	private:

		struct PoolInfo
		{
			std::vector<VkQueryPool> Pools;

			uint32_t QueriesPerPool = 1;
			uint32_t NextFreeIndex = 0;
		};
		std::unordered_map<std::string, PoolInfo> m_PipelineStatisticsQueryPools;		
		PoolInfo m_TimestampQueryPools;



		struct QueryData
		{
			std::string PoolName;
			uint32_t IndexInPool = 0;
			uint32_t Count = 2;

			//Range (0, ..., Count-1)
			uint32_t CurrentIncrement = 0;
		};
		std::unordered_map<std::string, QueryData> m_TimestampQueries;
		std::unordered_map<std::string, QueryData> m_PipelineStatisticsQueries;
	};

}

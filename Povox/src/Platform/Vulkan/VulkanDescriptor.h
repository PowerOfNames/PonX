#pragma once
#include <vulkan/vulkan.h>

namespace Povox {

	class VulkanDescriptorAllocator
	{
	public:
		void Cleanup();
		bool Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);
		void ResetPool();

		struct PoolSizes
		{
			std::vector<std::pair<VkDescriptorType, float>> Sizes
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
			};
		};
	private:
		VkDescriptorPool CreatePool(const VulkanDescriptorAllocator::PoolSizes& sizes, uint32_t count, VkDescriptorPoolCreateFlags flags);
		VkDescriptorPool GrapPool();

	private:

		VkDescriptorPool m_CurrentPool = VK_NULL_HANDLE;
		PoolSizes m_Sizes;
		std::vector<VkDescriptorPool> m_FreePools;
		std::vector<VkDescriptorPool> m_UsedPools;
	};




	class VulkanDescriptorLayoutCache
	{
	public:
		void Cleanup();


		VkDescriptorSetLayout CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info);

		struct DescriptorLayoutInfo {
			std::vector<VkDescriptorSetLayoutBinding> Bindings;

			bool operator==(const DescriptorLayoutInfo& other) const;

			size_t hash() const;
		};

	private:

		struct DescriptorLayoutHash
		{
			std::size_t operator()(const DescriptorLayoutInfo& k) const
			{
				return k.hash();
			}
		};

		std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_LayoutCache;
	};



	class VulkanDescriptorBuilder
	{
	public:
		static VulkanDescriptorBuilder Begin(VulkanDescriptorLayoutCache* layoutCache, VulkanDescriptorAllocator* allocator);


		VulkanDescriptorBuilder& BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
		VulkanDescriptorBuilder& BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

		bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
		bool Build(VkDescriptorSet& set);

	private:
		std::vector<VkWriteDescriptorSet> m_Writes;
		std::vector<VkDescriptorSetLayoutBinding> m_Bindings;

		VulkanDescriptorLayoutCache* m_Cache;
		VulkanDescriptorAllocator* m_Allocator;
	};
}

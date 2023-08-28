#pragma once
#include <vulkan/vulkan.h>

namespace Povox {

	class VulkanDescriptorAllocator
	{
	public:
		VulkanDescriptorAllocator() = default;
		~VulkanDescriptorAllocator();
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
		VulkanDescriptorLayoutCache() = default;
		~VulkanDescriptorLayoutCache();
		void Cleanup();


		VkDescriptorSetLayout CreateDescriptorLayout(const VkDescriptorSetLayoutCreateInfo* info, const std::string& name);
		VkDescriptorSetLayout GetLayout(const std::string& name);

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
		std::unordered_map<std::string, VkDescriptorSetLayout> m_UniqueLayouts;
	};




	class VulkanDescriptorBuilder
	{
	public:
		static VulkanDescriptorBuilder Begin(Ref<VulkanDescriptorLayoutCache> layoutCache, Ref<VulkanDescriptorAllocator> allocator);


		VulkanDescriptorBuilder& BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
		VulkanDescriptorBuilder& BindBuffer(VkDescriptorSetLayoutBinding newBinding, VkDescriptorBufferInfo* bufferInfo);
		VulkanDescriptorBuilder& BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
		VulkanDescriptorBuilder& BindImage(VkDescriptorSetLayoutBinding newBinding, VkDescriptorImageInfo* imageInfo);

		bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, const std::string& debugName = "");
		bool Build(VkDescriptorSet& set, const std::string& debugName = "");
		bool BuildNoWrites(VkDescriptorSet& set, VkDescriptorSetLayout layout, const std::string& debugName = "");

	private:
		std::vector<VkWriteDescriptorSet> m_Writes;
		std::vector<VkDescriptorSetLayoutBinding> m_Bindings;

		Ref<VulkanDescriptorLayoutCache> m_Cache;
		Ref<VulkanDescriptorAllocator> m_Allocator;
	};
}

#include "pxpch.h"
#include "VulkanDescriptor.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"


namespace Povox
{

	VulkanDescriptorAllocator::~VulkanDescriptorAllocator()
	{
		Cleanup();
	}

// ------------- Allocator -------------
	void VulkanDescriptorAllocator::Cleanup()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		for (auto pool : m_FreePools)
		{
			vkDestroyDescriptorPool(device, m_CurrentPool, nullptr);
		}
		for (auto pool : m_UsedPools)
		{
			vkDestroyDescriptorPool(device, m_CurrentPool, nullptr);
		}
	}

	bool VulkanDescriptorAllocator::Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
	{
		if (m_CurrentPool == VK_NULL_HANDLE)
		{
			m_CurrentPool = GrapPool();
			m_UsedPools.push_back(m_CurrentPool);
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;

		allocInfo.descriptorPool = m_CurrentPool;
		allocInfo.pSetLayouts = &layout;
		allocInfo.descriptorSetCount = 1;

		VkResult allocResult = vkAllocateDescriptorSets(VulkanContext::GetDevice()->GetVulkanDevice(), &allocInfo, set);
		bool needsReallocate = false;

		switch (allocResult)
		{
		case VK_SUCCESS: return true;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			needsReallocate = true;	break;

		return false;
		}

		if (needsReallocate)
		{
			m_CurrentPool = GrapPool();
			m_UsedPools.push_back(m_CurrentPool);
			allocInfo.descriptorPool = m_CurrentPool;

			PX_CORE_VK_ASSERT(vkAllocateDescriptorSets(VulkanContext::GetDevice()->GetVulkanDevice(), &allocInfo, set), VK_SUCCESS, "DescriptorSetAllocation failed after reAlloc try!");
			return true;
		}
		return false;
	}

	void VulkanDescriptorAllocator::ResetPool()
	{
		for (auto pool : m_UsedPools)
		{
			vkResetDescriptorPool(VulkanContext::GetDevice()->GetVulkanDevice(), pool, 0);
			m_FreePools.push_back(pool);
		}

		m_UsedPools.clear();
		m_CurrentPool = VK_NULL_HANDLE;
	}

	VkDescriptorPool VulkanDescriptorAllocator::CreatePool(const VulkanDescriptorAllocator::PoolSizes& sizes, uint32_t count, VkDescriptorPoolCreateFlags flags)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.reserve(sizes.Sizes.size());
		for (auto sz : sizes.Sizes)
		{
			poolSizes.push_back({ sz.first, static_cast<uint32_t>(sz.second * count) });
		}

		VkDescriptorPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		info.pPoolSizes = poolSizes.data();
		info.flags = flags;
		info.maxSets = count;

		VkDescriptorPool pool;
		PX_CORE_VK_ASSERT(vkCreateDescriptorPool(VulkanContext::GetDevice()->GetVulkanDevice(), &info, nullptr, &pool), VK_SUCCESS, "Failed to create DescriptorPool!");
		return pool;
	}

	VkDescriptorPool VulkanDescriptorAllocator::GrapPool()
	{
		if (m_FreePools.size() > 0)
		{
			VkDescriptorPool pool = m_FreePools.back();
			m_FreePools.pop_back();
			return pool;
		}
		else
		{
			return CreatePool(m_Sizes, 1000, 0);
		}
	}


// ------------- LayoutCache -------------
	VulkanDescriptorLayoutCache::~VulkanDescriptorLayoutCache()
	{
		Cleanup();
	}

	void VulkanDescriptorLayoutCache::Cleanup()
	{
		for (auto pair : m_LayoutCache)
		{
			vkDestroyDescriptorSetLayout(VulkanContext::GetDevice()->GetVulkanDevice(), pair.second, nullptr);
		}
	}

	VkDescriptorSetLayout VulkanDescriptorLayoutCache::CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info)
	{
		DescriptorLayoutInfo layoutInfo;
		layoutInfo.Bindings.reserve(info->bindingCount);
		bool isSorted = true;
		int lastBinding = -1;

		for (uint32_t i = 0; i < info->bindingCount; i++)
		{
			layoutInfo.Bindings.push_back(info->pBindings[i]);

			if (info->pBindings[i].binding > lastBinding)
			{
				lastBinding = info->pBindings[i].binding;
			}
			else
			{
				isSorted = false;
			}
		}

		if (!isSorted)
			std::sort(layoutInfo.Bindings.begin(), layoutInfo.Bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b)
			{
				return a.binding < b.binding;
			});

		auto it = m_LayoutCache.find(layoutInfo);
		if (it != m_LayoutCache.end())
		{
			return (*it).second;
		}
		else
		{
			VkDescriptorSetLayout layout;
			vkCreateDescriptorSetLayout(VulkanContext::GetDevice()->GetVulkanDevice(), info, nullptr, &layout);

			m_LayoutCache[layoutInfo] = layout;
			return layout;
		}		
	}

	bool VulkanDescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
	{
		if (other.Bindings.size() != this->Bindings.size())
		{
			return false;
		}
		else
		{
			for (uint32_t i = 0; i < this->Bindings.size(); i++)
			{
				if (other.Bindings[i].binding != this->Bindings[i].binding)
					return false;
				if (other.Bindings[i].descriptorCount != this->Bindings[i].descriptorCount)
					return false;
				if (other.Bindings[i].descriptorType != this->Bindings[i].descriptorType)
					return false;
				if (other.Bindings[i].stageFlags != this->Bindings[i].stageFlags)
					return false;
			}
		}
		return true;
	}

	size_t VulkanDescriptorLayoutCache::DescriptorLayoutInfo::hash() const
	{		
		std::size_t result = std::hash<std::size_t>()(this->Bindings.size());

		for (const VkDescriptorSetLayoutBinding& binding : this->Bindings)
		{
			std::size_t bindingHash = binding.binding | binding.descriptorType << 8 | binding.descriptorCount << 16 | binding.stageFlags << 24;
			result ^= std::hash<std::size_t>()(bindingHash);
		}
		return result;
	}

// ------------- DescriptorBuilder -------------

	VulkanDescriptorBuilder VulkanDescriptorBuilder::Begin(Ref<VulkanDescriptorLayoutCache> layoutCache, Ref<VulkanDescriptorAllocator> allocator)
	{
		VulkanDescriptorBuilder builder;
		builder.m_Cache = layoutCache;
		builder.m_Allocator = allocator;

		return builder;
	}

	VulkanDescriptorBuilder& VulkanDescriptorBuilder::BindBuffer(VkDescriptorSetLayoutBinding newBinding, VkDescriptorBufferInfo* bufferInfo)
	{
		m_Bindings.push_back(newBinding);

		VkWriteDescriptorSet newWrite{};
		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;

		newWrite.descriptorCount = newBinding.descriptorCount;
		newWrite.descriptorType = newBinding.descriptorType;
		newWrite.pBufferInfo = bufferInfo;
		newWrite.dstBinding = newBinding.binding;

		m_Writes.push_back(newWrite);

		return *this;
	}

	VulkanDescriptorBuilder& VulkanDescriptorBuilder::BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
	{
		VkDescriptorSetLayoutBinding newBinding{};
		newBinding.binding = binding;
		newBinding.descriptorCount = 1;
		newBinding.descriptorType = type;
		newBinding.pImmutableSamplers = nullptr;
		newBinding.stageFlags = stageFlags;

		m_Bindings.push_back(newBinding);

		VkWriteDescriptorSet newWrite{};
		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;

		newWrite.descriptorCount = 1;
		newWrite.descriptorType = type;
		newWrite.pBufferInfo = bufferInfo;
		newWrite.dstBinding = binding;

		m_Writes.push_back(newWrite);

		return *this;
	}

	VulkanDescriptorBuilder& VulkanDescriptorBuilder::BindImage(VkDescriptorSetLayoutBinding newBinding, VkDescriptorImageInfo* imageInfo)
	{
		m_Bindings.push_back(newBinding);

		VkWriteDescriptorSet newWrite{};
		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;

		newWrite.descriptorCount = newBinding.descriptorCount;
		newWrite.descriptorType = newBinding.descriptorType;
		newWrite.pImageInfo = imageInfo;
		newWrite.dstBinding = newBinding.binding;

		m_Writes.push_back(newWrite);

		return *this;
	}
	VulkanDescriptorBuilder& VulkanDescriptorBuilder::BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
	{
		VkDescriptorSetLayoutBinding newBinding{};
		newBinding.binding = binding;
		newBinding.descriptorCount = 1;
		newBinding.descriptorType = type;
		newBinding.pImmutableSamplers = nullptr;
		newBinding.stageFlags = stageFlags;

		m_Bindings.push_back(newBinding);

		VkWriteDescriptorSet newWrite{};
		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;

		newWrite.descriptorCount = 1;
		newWrite.descriptorType = type;
		newWrite.pImageInfo = imageInfo;
		newWrite.dstBinding = binding;

		m_Writes.push_back(newWrite);

		return *this;
	}


	bool VulkanDescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, const std::string& debugName)
	{
		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pNext = nullptr;

		info.bindingCount = static_cast<uint32_t>(m_Bindings.size());
		info.pBindings = m_Bindings.data();

		layout = m_Cache->CreateDescriptorLayout(&info);

		bool success = m_Allocator->Allocate(&set, layout);
		if (!success)
			return false;

		for (auto& writes : m_Writes)
		{
			writes.dstSet = set;
		}
		//Debug
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
		nameInfo.objectHandle = (uint64_t)set;
		nameInfo.pObjectName = debugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);

		vkUpdateDescriptorSets(VulkanContext::GetDevice()->GetVulkanDevice(), static_cast<uint32_t>(m_Writes.size()), m_Writes.data(), 0, nullptr);
		return true;
	}


	bool VulkanDescriptorBuilder::Build(VkDescriptorSet& set, const std::string& debugName)
	{
		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pNext = nullptr;

		info.bindingCount = static_cast<uint32_t>(m_Bindings.size());
		info.pBindings = m_Bindings.data();

		VkDescriptorSetLayout layout = m_Cache->CreateDescriptorLayout(&info);

		bool success = m_Allocator->Allocate(&set, layout);
		if (!success)
			return false;

		for (auto& writes : m_Writes)
		{
			writes.dstSet = set;
		}
		//Debug
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
		nameInfo.objectHandle = (uint64_t)set;
		nameInfo.pObjectName = debugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);

		vkUpdateDescriptorSets(VulkanContext::GetDevice()->GetVulkanDevice(), static_cast<uint32_t>(m_Writes.size()), m_Writes.data(), 0, nullptr);
		return true;

		return false;
	}

}

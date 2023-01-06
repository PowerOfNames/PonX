#pragma once
#include "Povox/Core/Log.h"
#include "Povox/Renderer/Utilities.h"


#include <vk_mem_alloc.h>

#include <vector>
#include <optional>


namespace Povox {

	namespace VulkanUtils {


		static bool HasStencilComponent(VkFormat format)
		{
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}

		static VmaMemoryUsage GetVmaUsage(MemoryUtils::MemoryUsage usage)
		{
			switch (usage)
			{
			case MemoryUtils::MemoryUsage::UNDEFINED:	return VMA_MEMORY_USAGE_UNKNOWN;
			case MemoryUtils::MemoryUsage::GPU_ONLY:	return VMA_MEMORY_USAGE_GPU_ONLY;
			case MemoryUtils::MemoryUsage::CPU_ONLY:	return VMA_MEMORY_USAGE_CPU_ONLY;
			case MemoryUtils::MemoryUsage::UPLOAD:		return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case MemoryUtils::MemoryUsage::DOWNLOAD:	return VMA_MEMORY_USAGE_GPU_TO_CPU;
			case MemoryUtils::MemoryUsage::CPU_COPY:	return VMA_MEMORY_USAGE_CPU_COPY;
			}
		}

		static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags propertiyFlags)
		{
			VkPhysicalDeviceMemoryProperties memoryProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

			for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
			{
				// iterates over the properties and checks if the bit of a flag is set to 1
				if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertiyFlags) == propertiyFlags)
				{
					return i;
				}
			}
			PX_CORE_ASSERT(false, "Failed to find suitable memory type!");
		}

		static VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (VkFormat format : candidates)
			{
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
				if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features))
				{
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features))
				{
					return format;
				}
			}
			PX_CORE_ASSERT(false, "Failed to find supported format!");
		}

		static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice)
		{
			return FindSupportedFormat(physicalDevice, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		}
	}

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;

		bool IsAdequat() { return !Formats.empty() && !PresentModes.empty(); }
	};

	struct UploadContext
	{
		VkCommandPool CmdPoolGfx;
		VkCommandBuffer CmdBufferGfx;

		VkCommandPool CmdPoolTrsf;
		VkCommandBuffer CmdBufferTrsf;

		VkFence Fence;
	};
}

#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>

#include "Povox/Core/Log.h"

namespace Povox {

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;

		bool IsAdequat() { return !Formats.empty() && !PresentModes.empty(); }
	};

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;					// Does support windows
		std::optional<uint32_t> TransferFamily;					// not supported by all GPUs

		bool IsComplete() { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
		bool HasTransfer() { return TransferFamily.has_value(); }
	};

	struct QueueFamilies
	{
		VkQueue GraphicsQueue;
		VkQueue PresentQueue;
		VkQueue TransferQueue;
	};

	namespace VulkanUtils {

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
}

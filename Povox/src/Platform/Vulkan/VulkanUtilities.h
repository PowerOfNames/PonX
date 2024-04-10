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
			return VK_FORMAT_UNDEFINED;
		}

		static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice)
		{
			return FindSupportedFormat(physicalDevice, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		}

		static bool IsGraphicsStage(VkPipelineStageFlagBits2 stage)
		{
			VkPipelineStageFlags2 flags =
				VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT |
				//VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT |
				VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT |
				VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT |
				VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT |
				VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
				VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT |
				VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT |
				VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT |
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
				VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT |
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
				VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT |
				VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT |
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR |
				VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT |
				VK_PIPELINE_STAGE_2_INVOCATION_MASK_BIT_HUAWEI;

			return (stage & flags) == stage;
		}

		static bool IsComputeStage(VkPipelineStageFlagBits2 stage)
		{
			VkPipelineStageFlags2 flags =
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
				VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

			return (stage & flags) == stage;
		}

		static bool IsTransferStage(VkPipelineStageFlagBits2 stage)
		{
			VkPipelineStageFlags2 flags =
				VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT |
				VK_PIPELINE_STAGE_2_TRANSFER_BIT |
				VK_PIPELINE_STAGE_2_COPY_BIT |
				VK_PIPELINE_STAGE_2_BLIT_BIT |
				VK_PIPELINE_STAGE_2_RESOLVE_BIT |
				VK_PIPELINE_STAGE_2_CLEAR_BIT |
				VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR;

			return (stage & flags) == stage;
		}
	}

	struct DescriptorLayoutInfo {
		std::vector<VkDescriptorSetLayoutBinding> Bindings;

		void SortBindings()
		{
			std::sort(Bindings.begin(), Bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b)
				{
					return a.binding < b.binding;
				});
		}

		bool operator==(const DescriptorLayoutInfo& other) const;

		size_t hash() const;
	};

	struct DescriptorLayoutHash
	{
		std::size_t operator()(const DescriptorLayoutInfo& k) const
		{
			return k.hash();
		}
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;

		bool IsAdequat() { return !Formats.empty() && !PresentModes.empty(); }
	};

	struct UploadContext
	{
		VkCommandPool GraphicsCmdPool;
		VkCommandBuffer GraphicsCmdBuffer;
		VkFence GraphicsFence;

		VkCommandPool TransferCmdPool;
		VkCommandBuffer TransferCmdBuffer;
		VkFence TransferFence;

		VkCommandPool ComputeCmdPool;
		VkCommandBuffer ComputeCmdBuffer;
		VkFence ComputeFence;
	};

	enum class QueueFamilyOwnership
	{
		QFO_UNDEFINED,
		QFO_TRANSFER,
		QFO_GRAPHICS,
		QFO_COMPUTE
	};
	namespace ToStringUtility {
		static std::string QueueFamilyOwnershipToString(QueueFamilyOwnership type)
		{
			switch (type)
			{
				case QueueFamilyOwnership::QFO_UNDEFINED: return "QueueFamilyOwnership::QFO_UNDEFINED";
				case QueueFamilyOwnership::QFO_GRAPHICS: return "QueueFamilyOwnership::QFO_GRAPHICS";
				case QueueFamilyOwnership::QFO_TRANSFER: return "QueueFamilyOwnership::QFO_TRANSFER";
				case QueueFamilyOwnership::QFO_COMPUTE: return "QueueFamilyOwnership::QFO_COMPUTE";
				default:
				{
					PX_CORE_WARN("QueueFamilyOwnership not defined!");
					return "Missing QueueFamilyOwnership!";
				}
			}
		}
	}
}

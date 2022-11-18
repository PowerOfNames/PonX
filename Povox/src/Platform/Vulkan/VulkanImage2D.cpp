#include "pxpch.h"
#include "VulkanImage2D.h"

#include "VulkanCommands.h"
#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "VulkanDebug.h"
#include "VulkanInitializers.h"


#include <stb_image.h>


namespace Povox {

	VulkanImage2D::VulkanImage2D(const ImageSpecification& spec)
	{
		VkExtent3D extent;
		extent.width = spec.Width;
		extent.height = spec.Height;
		extent.depth = 1;
		
		m_Allocation = CreateAllocation(extent, VulkanUtils::GetVulkanImageFormat(spec.Format), VulkanUtils::GetVulkanTiling(spec.Tiling),
			VulkanUtils::GetVulkanImageUsages(spec.Usages), VulkanUtils::GetVmaUsage(spec.Memory));
		
		CreateImageView();
	}

	VulkanImage2D::VulkanImage2D(const char* path, VkFormat format)
	{
		int width, height, channels;
		stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
		PX_CORE_ASSERT(pixels, "Failed to load texture!");
		VkDeviceSize imageSize = width * height * 4;

		PX_CORE_TRACE("Image width : '{0}', height '{1}'", width, height);

		AllocatedBuffer stagingBuffer = VulkanBuffer::CreateAllocation(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		void* data;
		vmaMapMemory(VulkanContext::GetAllocator(), stagingBuffer.Allocation, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(VulkanContext::GetAllocator(), stagingBuffer.Allocation);

		stbi_image_free(pixels);

		VkExtent3D extent;
		extent.width = static_cast<uint32_t>(width);
		extent.height = static_cast<uint32_t>(height);
		extent.depth = 1;

		m_Allocation = CreateAllocation(extent, format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU);

		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = m_Allocation.Image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage;
				VkPipelineStageFlags dstStage;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER, [=](VkCommandBuffer cmd)
			{
				VkBufferImageCopy region{};
				region.bufferOffset = 0;
				region.bufferImageHeight = 0;
				region.bufferRowLength = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

				vkCmdCopyBufferToImage(cmd, stagingBuffer.Buffer, m_Allocation.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			});
		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = m_Allocation.Image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage;
				VkPipelineStageFlags dstStage;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});

		vmaDestroyBuffer(VulkanContext::GetAllocator(), stagingBuffer.Buffer, stagingBuffer.Allocation);
	}

	VulkanImage2D::~VulkanImage2D()
	{
		vmaDestroyImage(VulkanContext::GetAllocator(), m_Allocation.Image, m_Allocation.Allocation);
	}

	void VulkanImage2D::Destroy()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		
		if(m_Sampler)
			vkDestroySampler(device, m_Sampler, nullptr);
		if(m_View)
			vkDestroyImageView(device, m_View, nullptr);
		if (m_Allocation.Image)
			vmaDestroyImage(VulkanContext::GetAllocator(), m_Allocation.Image, m_Allocation.Allocation);
	}

	AllocatedImage VulkanImage2D::CreateAllocation(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, VkImageLayout initialLayout)
	{
		VkImageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.imageType = VK_IMAGE_TYPE_2D;

		info.extent = extent;
		info.format = format;	// may not be supported by all hardware

		info.arrayLayers = 1;
		info.mipLevels = 1;
		info.tiling = tiling;
		info.initialLayout = initialLayout;
		info.usage = usage;	// used as destination from the buffer to image, and the shader needs to sample from it
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// exclusive means it will only be used by one queue family (graphics and therefore also transfer possible)
		info.samples = VK_SAMPLE_COUNT_1_BIT;	// related to multi sampling -> in attachments
		info.flags = 0;	// optional, can be used for sparse textures, in for examples for voxel terrain, to avoid using memory with AIR
		
		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = memUsage;
		if (allocationInfo.usage == VMA_MEMORY_USAGE_GPU_ONLY)
			allocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


		//allocation needs to be cleaned up later
		AllocatedImage output;
		PX_CORE_VK_ASSERT(vmaCreateImage(VulkanContext::GetAllocator(), &info, &allocationInfo, &output.Image, &output.Allocation, nullptr), VK_SUCCESS, "Failed to create Image!");
		return output;
	}

	void VulkanImage2D::CreateImageView()
	{
		VkImageAspectFlags mask = 0;
		for (auto& usage : m_Specification.Usages.Usages)
		{
			switch (usage)
			{
			case ImageUsage::COLOR_ATTACHMENT: mask |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; break;
			case ImageUsage::DEPTH_ATTACHMENT: mask |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; break;
			case ImageUsage::INPUT_ATTACHMENT: mask |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT; break;
			case ImageUsage::STORAGE: mask |= VK_IMAGE_USAGE_STORAGE_BIT; break;
			case ImageUsage::SAMPLED: mask |= VK_IMAGE_USAGE_SAMPLED_BIT; break;
			default: PX_CORE_ASSERT(true, "Usage not covered");
			}
		}
		if (m_Specification.CopySrc)
			mask |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (m_Specification.CopyDst)
			mask |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkImageViewCreateInfo imageInfo = VulkanInits::CreateImageViewInfo(VulkanUtils::GetVulkanImageFormat(m_Specification.Format), m_Allocation.Image, mask);
		PX_CORE_VK_ASSERT(vkCreateImageView(VulkanContext::GetDevice()->GetVulkanDevice(), &imageInfo, nullptr, &m_View), VK_SUCCESS, "Failed to create image view!");
	}


	void VulkanImage2D::CreateDescriptorSet()
	{

	}

	void VulkanImage2D::CreateSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(VulkanContext::GetDevice()->GetPhysicalDevice(), &properties);
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_TRUE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.minLod = 0.0f;

		PX_CORE_VK_ASSERT(vkCreateSampler(VulkanContext::GetDevice()->GetVulkanDevice(), &samplerInfo, nullptr, &m_Sampler), VK_SUCCESS, "Failed to create texture sampler!");
	}

}

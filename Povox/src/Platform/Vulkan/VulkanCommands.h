#pragma once

#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanUtilities.h"

#include "Povox/Core/Core.h"

#include <vulkan/vulkan.h>

namespace Povox {

	class VulkanCommandControl
	{
	public:
		enum class SubmitType
		{
			SUBMIT_TYPE_UNDEFINED,
			SUBMIT_TYPE_GRAPHICS,
			SUBMIT_TYPE_TRANSFER
		};

		static void ImmidiateSubmit(SubmitType submitType, std::function<void(VkCommandBuffer cmd)>&& function);

		inline const Ref<UploadContext> GetUploadContext() { return m_UploadContext; }
		Ref<UploadContext> CreateUploadContext();

	private:
		static Ref<UploadContext> m_UploadContext;
	};
}

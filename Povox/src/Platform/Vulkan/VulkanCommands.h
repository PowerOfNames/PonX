#pragma once

#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanUtilities.h"

#include "Povox/Core/Core.h"

#include <vulkan/vulkan.h>

namespace Povox {

	struct CommandControlState
	{
		bool IsInitialized = false;
	};

	class VulkanCommandControl
	{
	public:
		VulkanCommandControl() = default;
		~VulkanCommandControl();

		void Destroy();
		Ref<UploadContext> CreateUploadContext();

		enum class SubmitType
		{
			SUBMIT_TYPE_UNDEFINED,
			SUBMIT_TYPE_GRAPHICS_GRAPHICS,
			SUBMIT_TYPE_GRAPHICS_TRANSFER,
			SUBMIT_TYPE_TRANSFER_TRANSFER,
			SUBMIT_TYPE_TRANSFER_GRAPHICS,
			SUBMIT_TYPE_GRAPHICS_COMPUTE,
			SUBMIT_TYPE_COMPUTE_COMPUTE,
			SUBMIT_TYPE_COMPUTE_TRANSFER,
			SUBMIT_TYPE_TRANSFER_COMPUTE,
			SUBMIT_TYPE_COMPUTE_GRAPHICS
		};

		struct OwnershipTransferConfig
		{
			uint32_t SrcQueueIndex;
			uint32_t DstQueueIndex;
			SubmitType SubmissionType;
		};

		static void ImmidiateSubmitOwnershipTransfer(SubmitType submitType,
			std::function<void(VkCommandBuffer releasingCmd)> && releaseFunction,
			std::function<void(VkCommandBuffer acquireCmd)> && acquireFunction);

		static void ImmidiateSubmit(SubmitType submitType, std::function<void(VkCommandBuffer cmd)>&& function);

		inline const Ref<UploadContext> GetUploadContext() { return s_UploadContext; }

		static inline CommandControlState& GetState() { return s_CommandControlState; }
		static inline bool IsInitialized() { return s_CommandControlState.IsInitialized; }

		static const OwnershipTransferConfig DetermineSubmitType(QueueFamilyOwnership currentOwner, QueueFamilyOwnership targetOwner);

	private:
		static CommandControlState s_CommandControlState;
		static Ref<UploadContext> s_UploadContext;
	};
}

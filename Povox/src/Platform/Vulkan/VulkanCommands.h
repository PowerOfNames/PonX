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
		~VulkanCommandControl() = default;

		enum class SubmitType
		{
			SUBMIT_TYPE_UNDEFINED,
			SUBMIT_TYPE_GRAPHICS,
			SUBMIT_TYPE_TRANSFER
		};

		static void ImmidiateSubmit(SubmitType submitType, std::function<void(VkCommandBuffer cmd)>&& function);

		inline const Ref<UploadContext> GetUploadContext() { return s_UploadContext; }
		Ref<UploadContext> CreateUploadContext();

		static inline CommandControlState& GetState() { return s_CommandControlState; }
		static inline bool IsInitialized() { return s_CommandControlState.IsInitialized; }

	private:
		static CommandControlState s_CommandControlState;
		static Ref<UploadContext> s_UploadContext;
	};
}

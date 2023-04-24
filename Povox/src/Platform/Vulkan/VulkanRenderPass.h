#include "Platform/Vulkan/VulkanUtilities.h"

#include "Povox/Renderer/RenderPass.h"

namespace Povox {

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual void Recreate() override;

		virtual inline const RenderPassSpecification& GetSpecification() const override { return m_Specification; }

		inline VkRenderPass GetVulkanObj() const { return m_RenderPass; }

		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }
	private:
		RenderPassSpecification m_Specification;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};

}

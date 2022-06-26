#include "VulkanUtility.h"
#include "VulkanInitializers.h"

#include "Povox/Renderer/RenderPass.h"

namespace Povox {

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual inline const RenderPassSpecification& GetSpecification() const override { return m_Specification; }

		inline VkRenderPass GetVulkanRenderPass() const { return m_RenderPass; }
	private:
		RenderPassSpecification m_Specification;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;


	};

	class VulkanRenderPassBuilder
	{
	public:
		static VulkanRenderPassBuilder Begin();

		VulkanRenderPassBuilder& AddAttachment(VkAttachmentDescription attachment);
		VulkanRenderPassBuilder& CreateAndAddSubpass(VkPipelineBindPoint pipelineBindpoint, const std::vector<VkAttachmentReference>& colorRefs, VkAttachmentReference depthRef);
		VulkanRenderPassBuilder& CreateAndAddSubpass(VkPipelineBindPoint pipelineBindpoint, const std::vector<VkAttachmentReference>& colorRefs);
		VulkanRenderPassBuilder& AddDependency(VkSubpassDependency dependency);

		VkRenderPass Build();

	private:
		std::vector<VkAttachmentDescription> m_Attachments;
		std::vector<VkSubpassDescription> m_Subpasses;
		std::vector<VkSubpassDependency> m_Dependencies;
	};
}

#include "VulkanUtility.h"
#include "VulkanInitializers.h"

#include "Povox/Renderer/RenderPass.h"

namespace Povox {

	//Refactor the pools out
	class VulkanRenderPassPool
	{
	public:
		VulkanRenderPassPool();
		~VulkanRenderPassPool();

		void Clear();

		void AddRenderPass(const std::string& name, VkRenderPass pass);
		VkRenderPass GetRenderPass(const std::string& name);
		VkRenderPass RemoveRenderPass(const std::string& name);
		bool HasRenderPass(const std::string& name);
	private:
		std::unordered_map<std::string, VkRenderPass> m_Pool;
	};


	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual inline const RenderPassSpecification& GetSpecification() const override { return m_Specification; }

	private:
		RenderPassSpecification m_Specification;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;


	};

	class VulkanRenderPassBuilder
	{
	public:
		inline static void Init(const VulkanCoreObjects* coreObjects) { s_Core = coreObjects; }

		static VulkanRenderPassBuilder Begin();

		VulkanRenderPassBuilder& AddAttachment(VkAttachmentDescription attachment);
		VulkanRenderPassBuilder& CreateAndAddSubpass(VkPipelineBindPoint pipelineBindpoint, const std::vector<VkAttachmentReference>& colorRefs, VkAttachmentReference depthRef);
		VulkanRenderPassBuilder& CreateAndAddSubpass(VkPipelineBindPoint pipelineBindpoint, const std::vector<VkAttachmentReference>& colorRefs);
		VulkanRenderPassBuilder& AddDependency(VkSubpassDependency dependency);

		VkRenderPass Build();

	private:
		static const VulkanCoreObjects* s_Core;
		std::vector<VkAttachmentDescription> m_Attachments;
		std::vector<VkSubpassDescription> m_Subpasses;
		std::vector<VkSubpassDependency> m_Dependencies;
	};
}

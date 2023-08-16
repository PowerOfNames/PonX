#include "Platform/Vulkan/VulkanUtilities.h"

#include "Povox/Renderer/RenderPass.h"

namespace Povox {

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual void Recreate(uint32_t width, uint32_t height) override;

		virtual inline const RenderPassSpecification& GetSpecification() const override { return m_Specification; }

		inline VkRenderPass GetRenderPass() const { return m_RenderPass; }

		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }
	private:
		RenderPassSpecification m_Specification;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};


	/**
	 * This is just a helper class. There is no actual concept of a ComputePass in Vulkan. This help chaining resources together.
	 */
	class VulkanComputePass : public ComputePass
	{
	public:
		VulkanComputePass(const ComputePassSpecification& spec);
		virtual ~VulkanComputePass();

		virtual void Recreate() override;

		virtual inline const ComputePassSpecification& GetSpecification() const override { return m_Specification; }

		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }
	private:

		ComputePassSpecification m_Specification{};
	};
}

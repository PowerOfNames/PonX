#include "Platform/Vulkan/VulkanUtilities.h"

#include "Povox/Renderer/RenderPass.h"

namespace Povox {

	struct DescriptorSet
	{
		std::string Name = "SetName";
		uint32_t SetNumber = 0;
		uint32_t Bindings = 0;
		std::vector<VkDescriptorSet> Sets;
		VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
	};

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual void Recreate(uint32_t width, uint32_t height) override;

		virtual void BindResource(const std::string& name, Ref<UniformBuffer> resource) override;
		virtual void BindResource(const std::string& name, Ref<StorageBuffer> resource) override;

		virtual inline const RenderPassSpecification& GetSpecification() const override { return m_Specification; }

		inline const std::unordered_map<uint32_t, DescriptorSet>& GetDescriptors() const { return m_DescriptorSets; }

		inline VkRenderPass GetRenderPass() const { return m_RenderPass; }

		/**
		 * Writes the bound ShaderResources to the bound pipeline descriptor sets
		 */
		virtual void Bake() override;

		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }

	private:
		void CreateDescriptorSets();


	private:
		RenderPassSpecification m_Specification;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		std::unordered_map<std::string, Ref<ShaderResourceDescription>> m_AllShaderResourceDescs;

		//Temp here -> to me moved in DescriptorManager
		std::unordered_map<uint32_t, DescriptorSet> m_DescriptorSets;

		std::unordered_map<std::string, Ref<UniformBuffer>> m_UniformBuffers;
		std::unordered_map<std::string, Ref<StorageBuffer>> m_StorageBuffers;

		std::vector<std::string> m_InvalidResources;

	};

	class Image2D;
	/**
	 * This is just a helper class. There is no actual concept of a ComputePass in Vulkan. This help chaining resources together.
	 */
	class VulkanComputePass : public ComputePass
	{
	public:
		VulkanComputePass(const ComputePassSpecification& spec);
		virtual ~VulkanComputePass();

		virtual void Recreate() override;

		//virtual Ref<Image2D> GetFinalImage(uint32_t index) override;

		virtual inline const ComputePassSpecification& GetSpecification() const override { return m_Specification; }

		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }
	private:

		ComputePassSpecification m_Specification{};
	};
}

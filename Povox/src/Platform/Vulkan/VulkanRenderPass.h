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
		virtual void BindResource(const std::string& name, Ref<StorageImage> resource) override;
		virtual void BindResource(const std::string& name, Ref<Image2D> resource) override;

		/**
		 * Writes the bound ShaderResources to the bound pipeline descriptor sets
		 */
		virtual void Bake() override;

		inline virtual void SetPredecessor(Ref<RenderPass> predecessor) override { m_Specification.PredecessorPass = predecessor; }
		inline virtual void SetPredecessor(Ref<ComputePass> predecessor) override { m_Specification.PredecessorComputePass = predecessor; }
		inline virtual void SetSuccessor(Ref<RenderPass> successor) override { m_Specification.SuccessorPass = successor; }
		inline virtual void SetSuccessor(Ref<ComputePass> successor) override { m_Specification.SuccessorComputePass = successor; }

		inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
		inline const std::unordered_map<uint32_t, DescriptorSet>& GetDescriptors() const { return m_DescriptorSets; }


		virtual inline const RenderPassSpecification& GetSpecification() const override { return m_Specification; }
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
		std::unordered_map<std::string, Ref<StorageImage>> m_StorageImages;
		std::unordered_map<std::string, Ref<Image2D>> m_Images;

		std::vector<std::string> m_InvalidResources;

		//Resources shared between different passes/queues
		std::vector<std::string> m_SharedResources;

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

		virtual void BindResource(const std::string& name, Ref<UniformBuffer> resource) override;
		virtual void BindResource(const std::string& name, Ref<StorageBuffer> resource) override;
		virtual void BindResource(const std::string& name, Ref<StorageImage> resource) override;
		virtual void BindResource(const std::string& name, Ref<Image2D> resource) override;

		//virtual Ref<Image2D> GetFinalImage(uint32_t index) override;

		virtual void Bake() override;

		virtual inline void SetPredecessor(Ref<RenderPass> predecessor) override { m_Specification.PredecessorPass = predecessor; }
		virtual inline void SetPredecessor(Ref<ComputePass> predecessor) override { m_Specification.PredecessorComputePass = predecessor; }
		virtual inline void SetSuccessor(Ref<RenderPass> successor) override { m_Specification.SuccessorPass = successor; }
		virtual inline void SetSuccessor(Ref<ComputePass> successor) override { m_Specification.SuccessorComputePass = successor; }

		inline const std::unordered_map<uint32_t, DescriptorSet>& GetDescriptorSets() const { return m_DescriptorSets; }

		virtual inline const ComputePassSpecification& GetSpecification() const override { return m_Specification; }
		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }

	private:
		void CreateDescriptorSets();

	private:

		ComputePassSpecification m_Specification{};

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		std::unordered_map<std::string, Ref<ShaderResourceDescription>> m_AllShaderResourceDescs;

		//Temp here -> to me moved in DescriptorManager
		std::unordered_map<uint32_t, DescriptorSet> m_DescriptorSets;

		std::unordered_map<std::string, Ref<UniformBuffer>> m_UniformBuffers;
		std::unordered_map<std::string, Ref<StorageBuffer>> m_StorageBuffers;
		std::unordered_map<std::string, Ref<StorageImage>> m_StorageImages;
		std::unordered_map<std::string, Ref<Image2D>> m_Images;

		std::vector<std::string> m_InvalidResources;
		std::vector<std::string> m_SharedResources;

	};
}

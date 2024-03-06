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

	class VulkanPass : public virtual GPUPass
	{
	public:
		VulkanPass();
		virtual ~VulkanPass() = default;

		virtual void BindInput(const std::string& name, Ref<ShaderResource>) override;
		virtual void BindOutput(const std::string& name, Ref<ShaderResource>) override;


		virtual void UpdateDescriptor(const std::string& name) override;
		/**
		 * Writes the bound ShaderResources to the bound pipeline descriptor sets
		 */
		virtual void Bake() override;		

		virtual inline void SetPredecessor(Ref<GPUPass> predecessor) override { m_PredecessorPass = predecessor; }
		virtual void SetSuccessor(Ref<GPUPass> successor) override { m_SuccessorPass = successor; }

		virtual PassType GetPassType() override { return m_Type; }

		inline const std::map<uint32_t, DescriptorSet>& GetDescriptorSets() const { return m_DescriptorSets; }
		std::vector<uint32_t> GetDynamicOffsets(uint32_t currentFrameIndex);

	protected:
		void CreateDescriptorSets(const std::map<uint32_t, VkDescriptorSetLayout>& layoutMap);

		//minor bake, that checks and writes previously invalid descriptors
		void Validate();
			
	protected:
		PassType m_Type = PassType::UNDEFINED;

		Ref<GPUPass> m_PredecessorPass = nullptr;
		Ref<GPUPass> m_SuccessorPass = nullptr;

		std::string m_DebugName = "VulkanPass_Debug";

		// Resources needed by the bound shaders of this renderpass/pipeline
		std::unordered_map<std::string, Ref<ShaderResourceDescription>> m_AllShaderResourceDescs;

		// Resources bound using BindIn- or Output methods -> should ultimately be 1:1 with m_AllShaderResourceDescs
		std::unordered_map<std::string, Ref<ShaderResource>> m_BoundResources;


		//Temp here -> to me moved in DescriptorManager
		std::map<uint32_t, DescriptorSet> m_DescriptorSets;
		std::map<uint32_t, std::map<uint32_t, std::string>> m_DynamicDescriptors;

		// Resources that are not available during bake time
		std::unordered_map<std::string, Ref<ShaderResource>> m_InvalidResources;

		std::vector<std::string> m_Inputs;
		std::vector<std::string> m_Outputs;

		//std::unordered_map<ResourceKey, Ref<ShaderResource> m_BoundResources;
		// 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 (uint64_t)

	};

	class VulkanRenderPass : public RenderPass, public VulkanPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual void Recreate(uint32_t width, uint32_t height) override;		

		virtual inline const RenderPassSpecification& GetSpecification() const override { return m_Specification; }
		virtual inline RenderPassSpecification& GetSpecification() override { return m_Specification; }
		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }

		void UpdateResourceOwnership();

		inline VkRenderPass GetRenderPass() const { return m_RenderPass; }

	private:
		RenderPassSpecification m_Specification;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};


	/**
	 * This is just a helper class. There is no actual concept of a ComputePass in Vulkan. This help chaining resources together.
	 */
	class VulkanComputePass : public ComputePass, public VulkanPass
	{
	public:
		VulkanComputePass(const ComputePassSpecification& spec);
		virtual ~VulkanComputePass();

		virtual void Recreate(uint32_t width, uint32_t height) override;
		
		virtual inline const ComputePassSpecification& GetSpecification() const override { return m_Specification; }
		virtual inline ComputePassSpecification& GetSpecification() override { return m_Specification; }
		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }
		
		//virtual Ref<Image2D> GetFinalImage(uint32_t index) override;
		
		void UpdateResourceOwnership();

	private:
		ComputePassSpecification m_Specification{};
	};
}

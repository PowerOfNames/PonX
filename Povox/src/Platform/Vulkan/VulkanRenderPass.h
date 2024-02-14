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

		virtual void BindInput(const std::string& name, Ref<ShaderResource>) override;
		virtual void BindOutput(const std::string& name, Ref<ShaderResource>) override;


		virtual void UpdateDescriptor(const std::string& name) override;
		/**
		 * Writes the bound ShaderResources to the bound pipeline descriptor sets
		 */
		virtual void Bake() override;

		inline virtual void SetPredecessor(Ref<RenderPass> predecessor) override { m_Specification.PredecessorPass = predecessor; }
		inline virtual void SetPredecessor(Ref<ComputePass> predecessor) override { m_Specification.PredecessorComputePass = predecessor; }
		inline virtual void SetSuccessor(Ref<RenderPass> successor) override { m_Specification.SuccessorPass = successor; }
		inline virtual void SetSuccessor(Ref<ComputePass> successor) override { m_Specification.SuccessorComputePass = successor; }


		inline const std::map<uint32_t, DescriptorSet>& GetDescriptorSets() const { return m_DescriptorSets; }
		std::vector<uint32_t> GetDynamicOffsets(uint32_t currentFrameIndex);

		inline VkRenderPass GetRenderPass() const { return m_RenderPass; }
		virtual inline const RenderPassSpecification& GetSpecification() const override { return m_Specification; }
		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }

	private:
		void CreateDescriptorSets();

	private:
		RenderPassSpecification m_Specification;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

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

		//std::unordered_map<uint32_t, Ref<ShaderResource> m_BoundResources;
		// 0000 0000 0000 0000 0000 0000 0000 0010
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

		virtual void BindInput(const std::string& name, Ref<ShaderResource>) override;
		virtual void BindOutput(const std::string& name, Ref<ShaderResource>) override;

		virtual void UpdateDescriptor(const std::string& name) override;

		//virtual Ref<Image2D> GetFinalImage(uint32_t index) override;

		virtual void Bake() override;

		virtual inline void SetPredecessor(Ref<RenderPass> predecessor) override { m_Specification.PredecessorPass = predecessor; }
		virtual inline void SetPredecessor(Ref<ComputePass> predecessor) override { m_Specification.PredecessorComputePass = predecessor; }
		virtual inline void SetSuccessor(Ref<RenderPass> successor) override { m_Specification.SuccessorPass = successor; }
		virtual inline void SetSuccessor(Ref<ComputePass> successor) override { m_Specification.SuccessorComputePass = successor; }

		inline const std::map<uint32_t, DescriptorSet>& GetDescriptorSets() { return m_DescriptorSets; }
		std::vector<uint32_t> GetDynamicOffsets(uint32_t currentFrameIndex);

		virtual inline const ComputePassSpecification& GetSpecification() const override { return m_Specification; }
		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }

	private:
		void CreateDescriptorSets();

		//minor bake, that checks and writes previously invalid descriptors
		void Validate();

	private:

		ComputePassSpecification m_Specification{};

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		std::unordered_map<std::string, Ref<ShaderResourceDescription>> m_AllShaderResourceDescs;

		//Temp here -> to me moved in DescriptorManager
		std::map<uint32_t, DescriptorSet> m_DescriptorSets;
		std::map<uint32_t, std::map<uint32_t, std::string>> m_DynamicDescriptors;

		std::unordered_map<std::string, Ref<ShaderResource>> m_BoundResources;
		std::unordered_map<std::string, Ref<ShaderResource>> m_InvalidResources;


		// for Render graph logic and resource ownership transitions
		std::vector<std::string> m_Inputs;
		std::vector<std::string> m_Outputs;

	};
}

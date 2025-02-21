#include "pxpch.h"
#include "VulkanShader.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanPipeline.h"

#include "Povox/Core/Time.h"
#include "Povox/Utils/FileUtility.h"
#include "Povox/Utils/ShaderResource.h"

#include <cstdint>
#include <fstream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_common.hpp>
#include <spirv_reflect.h>

namespace Povox {

	namespace VulkanUtils {

		static VkShaderStageFlagBits VKShaderTypeFromString(const std::string& string)
		{
			if (string == "vertex")
				return VK_SHADER_STAGE_VERTEX_BIT;
			if (string == "fragment")
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			if (string == "geometry")
				return VK_SHADER_STAGE_GEOMETRY_BIT;
			if (string == "compute")
				return VK_SHADER_STAGE_COMPUTE_BIT;

			PX_CORE_ASSERT(false, "Shader type not found!");
		}

		static const char* VKShaderStageCachedVulkanFileExtension(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT: return ".cached_vulkan.vert";
				case VK_SHADER_STAGE_FRAGMENT_BIT: return ".cached_vulkan.frag";
				case VK_SHADER_STAGE_GEOMETRY_BIT: return ".cached_vulkan.geom";
				case VK_SHADER_STAGE_COMPUTE_BIT: return ".cached_vulkan.comp";
			}
			PX_CORE_ASSERT(false, "Shaderstage not defined");
			return "";
		}

		static shaderc_shader_kind VKShaderStageToShaderC(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT: return shaderc_glsl_vertex_shader;
				case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_glsl_fragment_shader;
				case VK_SHADER_STAGE_GEOMETRY_BIT: return shaderc_glsl_geometry_shader;
				case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_glsl_compute_shader;
			}

			PX_CORE_ASSERT(false, "Shaderstage not defined");
			return (shaderc_shader_kind)0;
		}

		static std::string VKShaderStageToString(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT: return "Vertex";
				case VK_SHADER_STAGE_FRAGMENT_BIT: return "Fragment";
				case VK_SHADER_STAGE_GEOMETRY_BIT: return "Geometry";
				case VK_SHADER_STAGE_COMPUTE_BIT: return "Compute";
			}

			PX_CORE_ASSERT(false, "Shaderstage not defined");
			return "Stage not supported!";
		}

		//Only for Vertex-attributes
		static uint32_t FormatSize(VkFormat format)
		{
			switch (format)
			{
				case VK_FORMAT_UNDEFINED: return 0;
				case VK_FORMAT_R32G32_SFLOAT: return 8;
				case VK_FORMAT_R32G32B32_SFLOAT: return 12;
				case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
				case VK_FORMAT_R8G8B8A8_SRGB: return 4;
				case VK_FORMAT_R32_SFLOAT: return 4;
				case VK_FORMAT_R32_SINT: return 4;
					
				default: return 0;
			}
		}

		static bool IsImageBinding(VkDescriptorType type)
		{
			switch (type)
			{
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return true;
				case VK_DESCRIPTOR_TYPE_SAMPLER: return true;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return true;
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return true;
				default: return false;
			}
		}

		static ShaderResourceType VulkanDescriptorTypeToShaderResourceType(VkDescriptorType type) {
			switch (type) {
				case VK_DESCRIPTOR_TYPE_SAMPLER: return ShaderResourceType::SAMPLER;
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return ShaderResourceType::COMBINED_IMAGE_SAMPLER;
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return ShaderResourceType::SAMPLED_IMAGE;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return ShaderResourceType::STORAGE_IMAGE;
				case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return ShaderResourceType::UNIFORM_TEXEL_BUFFER;
				case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return ShaderResourceType::STORAGE_TEXEL_BUFFER;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return ShaderResourceType::UNIFORM_BUFFER;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return ShaderResourceType::STORAGE_BUFFER;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return ShaderResourceType::UNIFORM_BUFFER_DYNAMIC;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return ShaderResourceType::STORAGE_BUFFER_DYNAMIC;
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return ShaderResourceType::INPUT_ATTACHMENT;
				case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return ShaderResourceType::ACCELERATION_STRUCTURE;
			}

			PX_CORE_ASSERT(true, "Unknown DescriptorType!");
			return ShaderResourceType::NONE;
		}
	}

	namespace SpirvUtils {

		static std::string ReflectErrorToString(SpvReflectResult result)
		{
			switch (result)
			{
				case SPV_REFLECT_RESULT_SUCCESS: return "SPV_REFLECT_RESULT_Success";
				case SPV_REFLECT_RESULT_NOT_READY : return "SPV_REFLECT_RESULT_NOT_READY";
				case SPV_REFLECT_RESULT_ERROR_PARSE_FAILED : return "SPV_REFLECT_RESULT__ERROR_PARSE_FAILED";
				case SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED : return "SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED";
				case SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED : return "SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED";
				case SPV_REFLECT_RESULT_ERROR_NULL_POINTER : return "SPV_REFLECT_RESULT_ERROR_NULL_POINTER";
				case SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR : return "SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR";
				case SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH : return "SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH";
				case SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND : return "SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE : return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER : return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF : return "SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE : return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW : return "SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS : return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION : return "SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION : return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA : return "SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE : return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT : return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT";
				case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE : return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE";
				default: return "SPVReflect case not known!";
			}
		}

		static std::string ReflectDecorationTypeDescriptionToString(const SpvReflectTypeDescription& type)
		{
			switch (type.op) {
				case SpvOpTypeVoid: {
					return "void";
					break;
				}
				case SpvOpTypeBool: {
					return "bool";
					break;
				}
				case SpvOpTypeInt: {
					if (type.traits.numeric.scalar.signedness)
						return "int";
					else
						return "uint";
				}
				case SpvOpTypeFloat: {
					switch (type.traits.numeric.scalar.width) {
						case 32:
							return "float";
						case 64:
							return "double";
						default:
							break;
					}
				}
				case SpvOpTypeStruct: {
					return "struct";
				}
				default: {
					break;
				}
			}
			return "";
		}

		static std::string ReflectDescriptorTypeToString(SpvReflectDescriptorType value) {
			switch (value) {
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return "VK_DESCRIPTOR_TYPE_SAMPLER";
				case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
				case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
				case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
			}
			// unhandled SpvReflectDescriptorType enum value
			return "VK_DESCRIPTOR_TYPE_???";
		}

		static ShaderResourceType ReflectDescriptorTypeToShaderResourceType(SpvReflectDescriptorType value, uint32_t set) {

			//TODO: Move to config: Set number for dynamic and static descriptors -> 1 is dynamic atm
			switch (value) {
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return ShaderResourceType::SAMPLER;
				case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return ShaderResourceType::COMBINED_IMAGE_SAMPLER;
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return ShaderResourceType::SAMPLED_IMAGE;
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return ShaderResourceType::STORAGE_IMAGE;
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return ShaderResourceType::UNIFORM_TEXEL_BUFFER;
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return ShaderResourceType::STORAGE_TEXEL_BUFFER;
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				{
					if (set == 1)
						return ShaderResourceType::UNIFORM_BUFFER_DYNAMIC;
					return ShaderResourceType::UNIFORM_BUFFER;
				}
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				{
					if (set == 1)
						return ShaderResourceType::STORAGE_BUFFER_DYNAMIC;
					return ShaderResourceType::STORAGE_BUFFER;
				}
				case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return ShaderResourceType::INPUT_ATTACHMENT;
				case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return ShaderResourceType::ACCELERATION_STRUCTURE;
			}
			
			PX_CORE_ASSERT(true, "Unknown DescriptorType!");
			return ShaderResourceType::NONE;
		}

		static std::string ReflectShaderStageToString(SpvReflectShaderStageFlagBits stage) {
			switch (stage) {
				case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return "Reflected Vertex Stage";
				case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return "Reflected Fragment Stage";
				case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: return "Reflected Geometry Stage";
				case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return "Reflected Compute Stage";
				case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "Reflected Tessellation Control Stage";
				case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "Reflected Tessellation Eva Stage";
			}
			return "SPV_SHADER:_STAGE_???";
		}

		static ShaderStage ReflectShaderStageToStage(SpvReflectShaderStageFlagBits stage) {
			switch (stage)
			{
				case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return ShaderStage::VERTEX;
				case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return ShaderStage::FRAGMENT;
				case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: return ShaderStage::GEOMETRY;
				case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return ShaderStage::COMPUTE;
				case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return ShaderStage::TESSELLATION_CONTROL;
				case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return ShaderStage::TESSELLATION_EVALUATION;
			}
		}

		VkDescriptorType IsDynamic(VkDescriptorType type, uint32_t set)
		{
			if (set != 1)
				return type;

			switch (type)
			{
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				default:
				{
					PX_CORE_WARN("Type does not support DYNAMIC!");
					return type;
				}
			}
		}

	}

	VulkanShader::VulkanShader(const std::string& sources, const std::string& debugName/* = "Shader"*/)
		: m_DebugName(debugName)
	{
		PX_PROFILE_FUNCTION();


		auto shaderSourceCodeMap = PreProcess(sources);
		{
			Timer timer;
			CompileOrGetVulkanBinaries(shaderSourceCodeMap);
			PX_CORE_WARN("Shader compilation took {0}ms", timer.ElapsedMilliseconds());
			Reflect();
			PX_CORE_WARN("Shader compilation+reflection took {0}ms", timer.ElapsedMilliseconds());
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		for (auto& [stage, data] : m_SourceCodes)
		{
			createInfo.codeSize = data.size() * sizeof(uint32_t);
			createInfo.pCode = data.data();
			PX_CORE_VK_ASSERT(vkCreateShaderModule(VulkanContext::GetDevice()->GetVulkanDevice(), &createInfo, nullptr, &m_Modules[stage]), VK_SUCCESS, "Failed to create shader module!");
		}
	}
	VulkanShader::VulkanShader(const std::filesystem::path& filePath)
	{
		PX_PROFILE_FUNCTION();


		Utils::Shader::CreateVKCacheDirectoryIfNeeded();

		// Extract name from filepath
		const std::string stringPath = filePath.string();
		auto lastSlash = stringPath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = stringPath.rfind(".");
		auto count = lastDot == std::string::npos ? stringPath.size() - lastSlash : lastDot - lastSlash;
		m_DebugName = stringPath.substr(lastSlash, count);
		PX_CORE_WARN("Creating shader with name: {0}", m_DebugName);

		std::string sources = Utils::Shader::ReadFile(stringPath);
		auto shaderSourceCodeMap = PreProcess(sources);
		{
			Timer timer;
			CompileOrGetVulkanBinaries(shaderSourceCodeMap);
			PX_CORE_WARN("Shader compilation took {0}ms", timer.ElapsedMilliseconds());
			Reflect();
			PX_CORE_WARN("Shader compilation+reflection took {0}ms", timer.ElapsedMilliseconds());
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		for (auto& [stage, data] : m_SourceCodes)
		{
			createInfo.codeSize = data.size() * sizeof(uint32_t);
			createInfo.pCode = data.data();
			PX_CORE_VK_ASSERT(vkCreateShaderModule(VulkanContext::GetDevice()->GetVulkanDevice(), &createInfo, nullptr, &m_Modules[stage]), VK_SUCCESS, "Failed to create shader module!");
		}
	}

	void VulkanShader::Free()
	{
		VulkanContext::SubmitResourceFree([=]()
			{
				for (auto& module : m_Modules)
				{
					vkDestroyShaderModule(VulkanContext::GetDevice()->GetVulkanDevice(), module.second, nullptr);
				}
				m_Modules.clear();
				m_SourceCodes.clear();
				m_Handle = 0;
			});
	}

	bool VulkanShader::Recompile(const std::string& sources)
	{
		PX_PROFILE_FUNCTION();


		auto shaderSourceCodeMap = PreProcess(sources);
		{
			Timer timer;
			bool success = CompileOrGetVulkanBinaries(shaderSourceCodeMap, true);
			if(!success)
			{
				PX_CORE_WARN("VulkanShader::Recompile: Failed to compile binaries!");
				return false;
			}
			float compilationTime = timer.ElapsedMilliseconds();
			PX_CORE_WARN("VulkanShader:Recompile: (Re)compilation took {0}ms", compilationTime);
			success = Reflect();
			if(!success)
			{
				PX_CORE_WARN("VulkanShader::Recompile: Reflection lead to problems! Canceling recompilation!");
				return false;
			}
			PX_CORE_WARN("VulkanShader:Recompile: Reflection took {0}ms", timer.ElapsedMilliseconds() - compilationTime);
		}

		VkShaderModuleCreateInfo createInfo{};
		m_Modules.clear();
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		for (auto& [stage, data] : m_SourceCodes)
		{
			createInfo.codeSize = data.size() * sizeof(uint32_t);
			createInfo.pCode = data.data();
			PX_CORE_VK_ASSERT(vkCreateShaderModule(VulkanContext::GetDevice()->GetVulkanDevice(), &createInfo, nullptr, &m_Modules[stage]), VK_SUCCESS, "Failed to create shader module!");
		}

		if (m_Pipeline)
			m_Pipeline->Recreate(true);

		if (m_ComputePipeline)
			m_ComputePipeline->Recreate(true);

		return true;
	}

	

	//Creates a DescriptorSetLayout by reflecting the shader and allocates it from the Pool in the Context
	bool VulkanShader::Reflect()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		VkPhysicalDeviceProperties physicalProps = VulkanContext::GetDevice()->GetPhysicalDeviceProperties();
		bool debug = true;

		
		std::map<uint32_t, DescriptorLayoutInfo> setToLayoutInfos;

		for (auto&& [stage, data] : m_SourceCodes)
		{
			if (stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
			{
				if (!ReflectVertexStage(&data, debug))
					return false;				
			}

			SpvReflectShaderModule module{};
			SpvReflectResult result = spvReflectCreateShaderModule(sizeof(uint32_t) * data.size(), data.data(), &module);
			if (result != SPV_REFLECT_RESULT_SUCCESS)
			{
				SpirvUtils::ReflectErrorToString(result);
				PX_CORE_ERROR("ReflectionModule creation failed!");
				return false;
			}

		// DesciptorSets
			uint32_t count = 0;
			result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
			if (result != SPV_REFLECT_RESULT_SUCCESS)
			{
				PX_CORE_ERROR("DescriptorSets enumeration failed!");
				return false;
			}

			if (m_LayoutInfoHashes.find(stage) != m_LayoutInfoHashes.end())
			{
				if (m_LayoutInfoHashes.at(stage).size() != count)
				{
					PX_CORE_WARN("Shader::Reflect: Shader {0} - Stage {1} previously contained {2} descriptor sets, now {3}", 
						m_DebugName, 
						VulkanUtils::VKShaderStageToString(stage),
						m_LayoutInfoHashes.at(stage).size(), 
						count
					);
					return false;
				}
			}

			std::vector<SpvReflectDescriptorSet*> reflSets(count);
			result = spvReflectEnumerateDescriptorSets(&module, &count, reflSets.data());
			if (result != SPV_REFLECT_RESULT_SUCCESS)
			{
				PX_CORE_ERROR("DescriptorSets querying failed!");
				return false;
			}

			//For every set in shaderStage
			for (size_t i = 0; i < reflSets.size(); i++)
			{
				const SpvReflectDescriptorSet& reflSet = *(reflSets[i]);

				// abuse operator[] -> if not existant, creates an returns new. I therefor just need to check the bindings -> add binding if not existant, or update stage flag if existant
				DescriptorLayoutInfo& currentSetLayout = setToLayoutInfos[reflSet.set];
				for (size_t j = 0; j < reflSet.binding_count; j++)
				{
					const SpvReflectDescriptorBinding& reflBinding = *(reflSet.bindings[j]);

					const char* name;
					if (VulkanUtils::IsImageBinding(static_cast<VkDescriptorType>(reflBinding.descriptor_type)))
						name = reflBinding.name;
					else
						name = reflBinding.type_description->type_name;

					//check if binding is already in Set
					auto it = std::find_if(currentSetLayout.Bindings.begin(), currentSetLayout.Bindings.end(),
						[=](const VkDescriptorSetLayoutBinding& binding) {return binding.binding == reflBinding.binding; });
					if (it == currentSetLayout.Bindings.end())
					{
						//Not found -> add new binding
						VkDescriptorSetLayoutBinding binding{};
						binding.binding = reflBinding.binding;
						binding.descriptorType = SpirvUtils::IsDynamic(static_cast<VkDescriptorType>(reflBinding.descriptor_type), reflSet.set);
						binding.descriptorCount = 1;
						for (uint32_t dim = 0; dim < reflBinding.array.dims_count; dim++)
						{
							binding.descriptorCount *= reflBinding.array.dims[dim];
						}
						binding.stageFlags |= static_cast<VkShaderStageFlagBits>(module.shader_stage);

						currentSetLayout.Bindings.push_back(binding);

						Ref<ShaderResourceDescription> resource = CreateRef<ShaderResourceDescription>();
						resource->Set = reflSet.set;
						resource->Binding = reflBinding.binding;
						resource->Count = binding.descriptorCount;
						resource->Name = name;
						resource->ResourceType = VulkanUtils::VulkanDescriptorTypeToShaderResourceType(SpirvUtils::IsDynamic(static_cast<VkDescriptorType>(reflBinding.descriptor_type), reflSet.set));
						resource->Stages |= SpirvUtils::ReflectShaderStageToStage(module.shader_stage);

						m_ShaderResourceDescriptions[name] = std::move(resource);
					}
					else
					{						
						auto index = std::distance(currentSetLayout.Bindings.begin(), it);
						currentSetLayout.Bindings[index].stageFlags |= static_cast<VkShaderStageFlagBits>(module.shader_stage);
						
						m_ShaderResourceDescriptions[name]->Stages |= SpirvUtils::ReflectShaderStageToStage(module.shader_stage);
					}

					SortLayoutInfoBindings(currentSetLayout);
					m_LayoutInfoHashes[stage][reflSet.set] = currentSetLayout.hash();
				}

				if(!CheckDescriptorHash(stage, reflSet.set, setToLayoutInfos.at(reflSet.set)))
				{
					PX_CORE_WARN("Shader::Reflect: Shader: {}, Stage: {} - DescriptorSet {} does not match previous layout!", m_DebugName, VulkanUtils::VKShaderStageToString(stage), reflSet.set);
					return false;
				}
			}

			//Debug print descriptors
			#ifdef PX_DEBUG
			if (debug)
			{
				for (size_t i_sets = 0; i_sets < reflSets.size(); i_sets++)
				{
					SpvReflectDescriptorSet* reflSet = reflSets[i_sets];

					PX_CORE_INFO("Set: {0}", reflSet->set);
					PX_CORE_INFO("BindingCount: {0}", reflSet->binding_count);

					for (uint32_t i_bindings = 0; i_bindings < reflSet->binding_count; i_bindings++)
					{
						PX_CORE_INFO("Layout(Set = {}, Binding = {}) {} {}", reflSet->bindings[i_bindings]->set, reflSet->bindings[i_bindings]->binding,
							SpirvUtils::ReflectDescriptorTypeToString(reflSet->bindings[i_bindings]->descriptor_type).c_str(),	reflSet->bindings[i_bindings]->name);
						

						//Array
						if (reflSet->bindings[i_bindings]->array.dims_count > 0)
						{
							for (uint32_t k = 0; k < reflSet->bindings[i_bindings]->array.dims_count; k++)
							{
								PX_CORE_INFO("Array [{0}]", reflSet->bindings[i_bindings]->array.dims[k]);
							}
						}

						//Counter?
						if (reflSet->bindings[i_bindings]->uav_counter_binding != nullptr)
						{
							PX_CORE_INFO("Counter: (Set={0}, Binding={1}, Name={2})",
								reflSet->bindings[i_bindings]->uav_counter_binding->set,
								reflSet->bindings[i_bindings]->binding,
								reflSet->bindings[i_bindings]->name
							);
						}
					}
				}//for-end DescriptorSet-debug 	
			}
			#endif

			spvReflectDestroyShaderModule(&module);
		}//for-end stages

		

		// TODO: Hardcoded here. Put these in a file in the future as some lookup-table or something
		const std::unordered_map<uint32_t, std::string> setNames = {
			{0, "GlobalUBOs"},
			{1, "GlobalSSBOs"},
			{2, "Textures"},
			{3, "ObjectMaterials"}
		};
		for (auto const& [key, setLayout] : setToLayoutInfos)
		{
			m_DescriptorSetLayouts[key] = (VulkanContext::GetDescriptorLayoutCache()->CreateDescriptorLayout(setLayout, setNames.at(key)));
		}
		return true;
	}

	bool VulkanShader::ReflectVertexStage(const std::vector<uint32_t>* moduleData, bool printDebug/* = false*/)
	{
		SpvReflectShaderModule module{};
		SpvReflectResult result = spvReflectCreateShaderModule(sizeof(uint32_t) * moduleData->size(), moduleData->data(), &module);
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			SpirvUtils::ReflectErrorToString(result);
			PX_CORE_ERROR("ReflectionModule creation failed!");
			return false;
		}

		uint32_t count = 0;
		result = spvReflectEnumerateInputVariables(&module, &count, nullptr);
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			PX_CORE_ERROR("InputVariable enumeration failed!");
			return false;
		}

		if(count == 0 && m_VertexInputDescription.Attributes.size() != count)
		{
			PX_CORE_WARN("VulkanShader::ReflectVertexStage: Shader: {} InputAttributes({}) do not match with previous compilation ({})!", m_DebugName, count, m_VertexInputDescription.Attributes.size());
			return false;
		}
		if(count == m_VertexInputDescription.Attributes.size())
		{
			PX_CORE_INFO("VulkanShader::ReflectVertexStage: Shader: {} InputAttributes already done ({}/{})!", m_DebugName, count, m_VertexInputDescription.Attributes.size());
			return true;
		}

		std::vector<SpvReflectInterfaceVariable*> inputVars(count);
		result = spvReflectEnumerateInputVariables(&module, &count, inputVars.data());
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			PX_CORE_ERROR("InputVariable querying failed!");
			return false;
		}

		count = 0;
		result = spvReflectEnumerateOutputVariables(&module, &count, nullptr);
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			PX_CORE_ERROR("OutputVariable enumeration failed!");
			return false;
		}

		std::vector<SpvReflectInterfaceVariable*> outputVars(count);
		result = spvReflectEnumerateOutputVariables(&module, &count, outputVars.data());
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			PX_CORE_ERROR("OutputVariable querying failed!");
			return false;
		}

		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = 0;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		m_VertexInputDescription.Attributes.reserve(inputVars.size());
		for (size_t i = 0; i < inputVars.size(); i++)
		{
			const SpvReflectInterfaceVariable& reflectVar = *(inputVars[i]);
			//ignore build-in variables
			if (reflectVar.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
				continue;
			VkVertexInputAttributeDescription attributeDescription{};
			attributeDescription.location = reflectVar.location;
			attributeDescription.binding = bindingDescription.binding;
			attributeDescription.format = static_cast<VkFormat>(reflectVar.format);
			attributeDescription.offset = 0; //Later 
			m_VertexInputDescription.Attributes.push_back(attributeDescription);
		}
		//sort ascending
		std::sort(std::begin(m_VertexInputDescription.Attributes), std::end(m_VertexInputDescription.Attributes), [](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b)
			{
				return a.location < b.location;
			});
		//calculate offset
		for (auto& attribute : m_VertexInputDescription.Attributes)
		{
			uint32_t formatSize = VulkanUtils::FormatSize(attribute.format);
			attribute.offset = bindingDescription.stride;
			bindingDescription.stride += formatSize;
		}
		m_VertexInputDescription.Bindings.push_back(bindingDescription);

		//Debug Print Input and outputs
#ifdef PX_DEBUG
		if (printDebug)
		{
			PX_CORE_INFO("Entry Point: {0}", module.entry_point_name);
			PX_CORE_INFO("Source Language: {0}", spvReflectSourceLanguage(module.source_language));
			PX_CORE_INFO("Source Language Version: {0}", module.source_language_version);
			if (module.source_language == SpvSourceLanguageGLSL)
			{
				switch (module.shader_stage)
				{
					case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: PX_CORE_INFO("Shader Stage 'Vertex (VS)'"); break;
					case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT: PX_CORE_INFO("Shader Stage 'TesselationControl (HS)'"); break;
					case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: PX_CORE_INFO("Shader Stage 'TessellationEvalation (DS)'"); break;
					case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: PX_CORE_INFO("Shader Stage 'Geometry (GS)'"); break;
					case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: PX_CORE_INFO("Shader Stage 'Fragment (FS)'"); break;
					case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: PX_CORE_INFO("Shader Stage 'Compute (CS)'"); break;
				}
			}

			PX_CORE_INFO("Input Variables: ");
			for (size_t i = 0; i < m_VertexInputDescription.Attributes.size(); i++)
			{
				SpvReflectInterfaceVariable* var = inputVars[i];
				PX_CORE_INFO("layout(location = {}) in {} {}",
					var->location,
					SpirvUtils::ReflectDecorationTypeDescriptionToString(*var->type_description).c_str(),
					var->name
				);
			}

			PX_CORE_INFO("Out Variables: ");
			for (size_t i = 0; i < outputVars.size(); i++)
			{
				SpvReflectInterfaceVariable* var = outputVars[i];
				if (var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
					continue;
				PX_CORE_INFO("layout(location = {0}) out {1} {2}",
					var->location,
					SpirvUtils::ReflectDecorationTypeDescriptionToString(*var->type_description).c_str(),
					var->name
				);
			}
		}
#endif
		return true;
	}

	void VulkanShader::SortLayoutInfoBindings(DescriptorLayoutInfo& layoutInfo)
	{
		bool isSorted = true;
		int lastBinding = -1;
		for (uint32_t i = 0; i < layoutInfo.Bindings.size(); i++)
		{
			if (layoutInfo.Bindings[i].binding > lastBinding)
			{
				lastBinding = layoutInfo.Bindings[i].binding;
			}
			else
			{
				isSorted = false;
			}
		}
		if (!isSorted)
			layoutInfo.SortBindings();
	}

	bool VulkanShader::CheckDescriptorHash(VkShaderStageFlagBits stage, uint32_t set, const DescriptorLayoutInfo& layoutInfo)
	{
		if (m_LayoutInfoHashes.find(stage) == m_LayoutInfoHashes.end())
		{
			PX_CORE_WARN("VulkanShader::CheckDescriptorHash: Shader: {} - Stage {} not contained in previous shader reflection!", m_DebugName, VulkanUtils::VKShaderStageToString(stage));
			return false;
		}
		auto& sets = m_LayoutInfoHashes.at(stage);
		if (sets.find(set) == sets.end())
		{
			PX_CORE_WARN("VulkanShader::CheckDescriptorHash: Shader: {}, Stage {} - Set {} not contained in previous shader reflection!", m_DebugName, VulkanUtils::VKShaderStageToString(stage), set);
			return false;
		}
		return sets.at(set) == layoutInfo.hash();
	}

	//From Chernos GL code
	std::unordered_map<VkShaderStageFlagBits, std::string> VulkanShader::PreProcess(const std::string& sources)
	{
		PX_PROFILE_FUNCTION();


		std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = sources.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			// finds end of line AFTER the token
			// sets the begin of the 'type' to one after '#type'
			// cuts the type from the string

			size_t eol = sources.find_first_of("\r\n", pos);
			PX_CORE_ASSERT(eol != std::string::npos, "Syntax Error!");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = sources.substr(begin, eol - begin);
			PX_CORE_ASSERT(VulkanUtils::VKShaderTypeFromString(type), "Invalid shader type specified!");

			// finds the beginning of the shader string
			// sets position of the next '#type' token
			// cuts the shader code up until the next token or the end of the whole file
			size_t nextLine = sources.find_first_not_of("\r\n", eol);
			PX_CORE_ASSERT(nextLine != std::string::npos, "Syntax Error!");
			pos = sources.find(typeToken, nextLine);

			shaderSources[VulkanUtils::VKShaderTypeFromString(type)] = (pos == std::string::npos) ? sources.substr(nextLine) : sources.substr(nextLine, pos - nextLine);
		}
		return shaderSources;
	}

	bool VulkanShader::CompileOrGetVulkanBinaries(std::unordered_map<VkShaderStageFlagBits, std::string> sources, bool forceRecompile/* = false*/)
	{
		PX_PROFILE_FUNCTION();


		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetTargetSpirv(shaderc_spirv_version_1_6);

		const bool optimize = false;

#ifdef PX_DEBUG
		options.SetOptimizationLevel(shaderc_optimization_level_zero);
#else
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif
		std::filesystem::path cacheDirectory = Utils::Shader::GetVKCacheDirectory();
		

		auto& shaderData = m_SourceCodes;
		shaderData.clear();
		for (auto&& [stage, code] : sources)
		{
			std::filesystem::path cachedPath = cacheDirectory / (m_DebugName + VulkanUtils::VKShaderStageCachedVulkanFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open() && !forceRecompile) // happens if cache path exists
			{
				in.seekg(0, std::ios::end);				// places pointer to end of file
				auto size = in.tellg();					// position of pointer equals size
				in.seekg(0, std::ios::beg);				// places pointer back to start

				auto& data = shaderData[stage];			// gets a vector of uint32_t
				data.resize(size / sizeof(uint32_t));	// sets the size of the vector
				in.read((char*)data.data(), size);		// reads in and writes to data I guess
			}
			else
			{
 				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(code, VulkanUtils::VKShaderStageToShaderC(stage), m_DebugName.c_str(), options);
 				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
 				{
 					PX_CORE_ERROR(module.GetErrorMessage());
					return false;
 				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary | std::ofstream::trunc);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}
		return true;
	}
}

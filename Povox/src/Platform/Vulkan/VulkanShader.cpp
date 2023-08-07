#include "pxpch.h"
#include "VulkanShader.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"

#include "Povox/Utils/FileUtility.h"
#include "Povox/Core/Time.h"

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
	}

	namespace SpirvUtils {

		const char* GetBaseTypeAsString(const spirv_cross::SPIRType::BaseType& baseType)
		{
			switch (baseType)
			{
				case spirv_cross::SPIRType::BaseType::Boolean: return "Boolean";
				case spirv_cross::SPIRType::BaseType::Int: return "Int";
				case spirv_cross::SPIRType::BaseType::UInt: return "UInt";
				case spirv_cross::SPIRType::BaseType::Float: return "Float";
				case spirv_cross::SPIRType::BaseType::Struct: return "Struct";
				PX_CORE_ASSERT(true, "BaseType not covered or supported!");
			}
		}

	}

	namespace VulkanShaderUtils {

		std::string ReflectErrorToString(SpvReflectResult result)
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

		std::string ReflectDecorationTypeDescriptionToString(const SpvReflectTypeDescription& type)
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

		std::string ReflectDescriptorTypeToString(SpvReflectDescriptorType value) {
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

		std::string ReflectShaderStageToString(SpvReflectShaderStageFlagBits stage) {
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

		VkDescriptorType MapBindingToType(uint32_t bindingNumber, VkDescriptorType type)
		{
			switch (bindingNumber)
			{
				case 0:
				case 2:
				case 4:
				{
					switch (type)
					{
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return type;
						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return type;
						default: return type;
					}
				}
				case 1:
				case 3: 
				{
					switch (type)
					{
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
						default: return type;
					}
				}
				default: PX_CORE_ASSERT(true, "Binding number not covered");
			}
			return type;
		}

	}

	VulkanShader::VulkanShader(const std::string& filepath)
		: m_FilePath(filepath)
	{
		PX_PROFILE_FUNCTION();


		Utils::Shader::CreateVKCacheDirectoryIfNeeded();

		// Extract name from filepath
		auto lastSlash = m_FilePath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = m_FilePath.rfind(".");
		auto count = lastDot == std::string::npos ? m_FilePath.size() - lastSlash : lastDot - lastSlash;
		m_DebugName = m_FilePath.substr(lastSlash, count);
		PX_CORE_WARN("Creating shader with name: {0}", m_DebugName);

		std::string sources = Utils::Shader::ReadFile(filepath);
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
		m_RID = 0;
			});
	}

	//Creates a DescriptorSetLayout by reflecting the shader and allocates it from the Pool in the Context
	void VulkanShader::Reflect()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		VkPhysicalDeviceProperties physicalProps = VulkanContext::GetDevice()->GetPhysicalDeviceProperties();
		bool debug = false;

		struct DescriptorSetLayoutData {
			uint32_t SetNumber = 0;
			VkDescriptorSetLayoutCreateInfo CreateInfo{};
			std::vector<VkDescriptorSetLayoutBinding> Bindings;
		};
		std::unordered_map<uint32_t, DescriptorSetLayoutData> descriptorSetsData;


		uint32_t reflectionCheck = 0;
		std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setLayouts;
		for (auto&& [stage, data] : m_SourceCodes)
		{
			SpvReflectShaderModule module{};
			SpvReflectResult result = spvReflectCreateShaderModule(sizeof(uint32_t) * data.size(), data.data(), &module);
			if (result != SPV_REFLECT_RESULT_SUCCESS)
			{
				VulkanShaderUtils::ReflectErrorToString(result);
				PX_CORE_ASSERT(true, "ReflectionModule creation failed!");
			}

			uint32_t count = 0;
			result = spvReflectEnumerateInputVariables(&module, &count, nullptr);
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "InputVariable enumeration failed!");

			std::vector<SpvReflectInterfaceVariable*> inputVars(count);
			result = spvReflectEnumerateInputVariables(&module, &count, inputVars.data());
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "InputVariable querying failed!");

			count = 0;
			result = spvReflectEnumerateOutputVariables(&module, &count, nullptr);
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "OutputVariable enumeration failed!");

			std::vector<SpvReflectInterfaceVariable*> outputVars(count);
			result = spvReflectEnumerateOutputVariables(&module, &count, outputVars.data());
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "OutputVariable querying failed!");


			if (stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
			{
				VkVertexInputBindingDescription bindingDescription{};
				bindingDescription.binding = 0;
				bindingDescription.stride = 0;
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				m_VertexInputDescription.Attributes.reserve(inputVars.size());
				for (size_t i = 0; i < inputVars.size(); i++)//example says ++i Why?
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
				if (debug)
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
					for (size_t i = 0; i < inputVars.size(); i++)
					{
						SpvReflectInterfaceVariable* var = inputVars[i];
						PX_CORE_INFO("layout(location = {0}) in {1} {2}",
							var->location,
							VulkanShaderUtils::ReflectDecorationTypeDescriptionToString(*var->type_description).c_str(),
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
							VulkanShaderUtils::ReflectDecorationTypeDescriptionToString(*var->type_description).c_str(),
							var->name
						);
					}
				}
			#endif
			}//if-ShaderStage Vertex

		// DesciptorSets
			count = 0;
			result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "DescriptorSets enumeration failed!");

			std::vector<SpvReflectDescriptorSet*> reflSets(count);
			result = spvReflectEnumerateDescriptorSets(&module, &count, reflSets.data());
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "DescriptorSets querying failed!");

			//For every set in shaderStage
			for (size_t i = 0; i < reflSets.size(); i++)
			{
				const SpvReflectDescriptorSet& reflSet = *(reflSets[i]);

				//check if set is already in map
				if (descriptorSetsData.find(reflSet.set) == descriptorSetsData.end())
				{
					//not found -> create new layoutData
					DescriptorSetLayoutData ld;
					descriptorSetsData[reflSet.set] = ld;
					auto& newSetLayoutData = descriptorSetsData[reflSet.set];


					newSetLayoutData.CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					newSetLayoutData.CreateInfo.pNext = nullptr;
					newSetLayoutData.SetNumber = reflSet.set;

					//iterate through bindings of current set and add them (new set -> new bindings)
					for (size_t j = 0; j < reflSet.binding_count; j++)
					{
						const SpvReflectDescriptorBinding& reflBinding = *(reflSet.bindings[j]);

						VkDescriptorSetLayoutBinding binding{};
						binding.binding = reflBinding.binding;
						binding.descriptorType = VulkanShaderUtils::MapBindingToType(reflBinding.binding, static_cast<VkDescriptorType>(reflBinding.descriptor_type));
						binding.descriptorCount = 1;
						for (uint32_t dim = 0; dim < reflBinding.array.dims_count; dim++)
						{
							binding.descriptorCount *= reflBinding.array.dims[dim];
						}
						binding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);

						if (VulkanUtils::IsImageBinding(binding.descriptorType))
							m_DescriptorSetImageBindings.push_back(binding);
						else
							m_DescriptorSetBufferBindings.push_back(binding);

						newSetLayoutData.Bindings.push_back(binding);
					}
					
					newSetLayoutData.CreateInfo.bindingCount = static_cast<uint32_t>(newSetLayoutData.Bindings.size());
					newSetLayoutData.CreateInfo.pBindings = newSetLayoutData.Bindings.data();
				}
				else //set is in map
				{
					auto& currentSetLayoutData = descriptorSetsData[reflSet.set];

					//for every binding
					for (size_t j = 0; j < reflSet.binding_count; j++)
					{
						const SpvReflectDescriptorBinding& reflBinding = *(reflSet.bindings[j]);
						//check if binding is already in Set
						if (std::find_if(currentSetLayoutData.Bindings.begin(), currentSetLayoutData.Bindings.end(),
							[=](const VkDescriptorSetLayoutBinding& binding) {return binding.binding == reflBinding.binding; }) == currentSetLayoutData.Bindings.end())
						{
							//Not found -> add new binding
							VkDescriptorSetLayoutBinding binding{};
							binding.binding = reflBinding.binding;
							binding.descriptorType = VulkanShaderUtils::MapBindingToType(reflBinding.binding, static_cast<VkDescriptorType>(reflBinding.descriptor_type));
							binding.descriptorCount = 1;
							for (uint32_t dim = 0; dim < reflBinding.array.dims_count; dim++)
							{
								binding.descriptorCount *= reflBinding.array.dims[dim];
							}
							binding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);

							if (VulkanUtils::IsImageBinding(binding.descriptorType))
								m_DescriptorSetImageBindings.push_back(binding);
							else
								m_DescriptorSetBufferBindings.push_back(binding);
							currentSetLayoutData.Bindings.push_back(binding);
						}
						else
						{
							auto it = std::find_if(currentSetLayoutData.Bindings.begin(), currentSetLayoutData.Bindings.end(),
								[=](const VkDescriptorSetLayoutBinding& binding) {return binding.binding == reflBinding.binding; });
							if (it != currentSetLayoutData.Bindings.end())
							{
								auto index = std::distance(currentSetLayoutData.Bindings.begin(), it);
								currentSetLayoutData.Bindings[index].stageFlags |= static_cast<VkShaderStageFlagBits>(module.shader_stage);
							}
						}

						currentSetLayoutData.CreateInfo.bindingCount = static_cast<uint32_t>(currentSetLayoutData.Bindings.size());
						currentSetLayoutData.CreateInfo.pBindings = currentSetLayoutData.Bindings.data();

						/*PX_CORE_WARN("Set already there, binding added or updated");
						for (uint32_t k = 0; k < currentSetLayoutData.Bindings.size(); k++)
						{
							auto& binding = currentSetLayoutData.Bindings[k];
							PX_CORE_ERROR("Set: {0}", currentSetLayoutData.SetNumber);
							PX_CORE_ERROR("Binding: {0}", binding.binding);
							PX_CORE_ERROR("DescriptorCount: {0}", binding.descriptorCount);
							PX_CORE_ERROR("DescriptorType: {0}", binding.descriptorType);
						}
						for (uint32_t k = 0; k < currentSetLayoutData.CreateInfo.bindingCount; k++)
						{
							auto& binding = currentSetLayoutData.CreateInfo.pBindings[k];
							PX_CORE_WARN("Set: {0}", currentSetLayoutData.SetNumber);
							PX_CORE_WARN("Stage: {0}", binding.stageFlags);
							PX_CORE_WARN("SetBinding: {0}", binding.binding);
							PX_CORE_WARN("SetDescriptorCount: {0}", binding.descriptorCount);
							PX_CORE_WARN("SetDescriptorType: {0}", binding.descriptorType);
						}*/
					}
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
						PX_CORE_INFO("Layout(Binding = {0}, Set = {1}) {2} {3}", reflSet->bindings[i_bindings]->binding, reflSet->bindings[i_bindings]->set,
							VulkanShaderUtils::ReflectDescriptorTypeToString(reflSet->bindings[i_bindings]->descriptor_type).c_str(),
							reflSet->bindings[i_bindings]->name
						);

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
		m_DescriptorSetLayouts.reserve(descriptorSetsData.size());
		for (auto const& [key, setLayoutData] : descriptorSetsData)
		{
			VkDescriptorSetLayout layout;
			PX_CORE_VK_ASSERT(vkCreateDescriptorSetLayout(device, &setLayoutData.CreateInfo, nullptr, &layout), VK_SUCCESS, "Created descriptorSetLayout!");
			m_DescriptorSetLayouts.push_back(layout);
		}
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

	void VulkanShader::CompileOrGetVulkanBinaries(std::unordered_map<VkShaderStageFlagBits, std::string> sources)
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
		PX_CORE_WARN("Compiling shader ' {} ' ...", m_DebugName);
		for (auto&& [stage, code] : sources)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + VulkanUtils::VKShaderStageCachedVulkanFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open()) // happens if cache path exists
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
 				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(code, VulkanUtils::VKShaderStageToShaderC(stage), m_FilePath.c_str(), options);
 				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
 				{
 					PX_CORE_ERROR(module.GetErrorMessage());
 					PX_CORE_ASSERT(false, module.GetErrorMessage());
 				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}

			PX_CORE_WARN("Contains ShaderStage: {}", VulkanUtils::VKShaderStageToString(stage));
		}
	}
}

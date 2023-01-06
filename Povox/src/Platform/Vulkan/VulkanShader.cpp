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
				case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_glsl_compute_shader;
			}

			PX_CORE_ASSERT(false, "Shaderstage not defined");
			return (shaderc_shader_kind)0;
		}

		//Only for Vertex-attributes
		static uint32_t FormatSize(VkFormat format)
		{
			switch (format)
			{
				case VK_FORMAT_UNDEFINED: return 0;
				case VK_FORMAT_R8G8B8A8_SRGB: return 4;
				case VK_FORMAT_R16_UINT: return 2;
					
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
				case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return "Reflected Compute Stage";
				case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "Reflected Tessellation Control Stage";
				case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "Reflected Tessellation Eva Stage";
			}
			return "SPV_SHADER:_STAGE_???";
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


		PX_CORE_WARN("Starting to load shader from '{0}'", filepath);
		std::string sources = Utils::Shader::ReadFile(filepath);
		auto shaderSourceCodeMap = PreProcess(sources);

		{
			Timer timer;
			CompileOrGetVulkanBinaries(shaderSourceCodeMap);
			PX_CORE_WARN("Shader compilation took {0}ms", timer.ElapsedMilliseconds());
			Reflect();
			PX_CORE_WARN("Shader reflection took {0}ms", timer.ElapsedMilliseconds());
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

	//Creates a DescriptorSetLayout by reflecting the shade and allocates it form the Pool in the Context
	void VulkanShader::Reflect()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		VkPhysicalDeviceProperties physicalProps = VulkanContext::GetDevice()->GetPhysicalDeviceProperties();
				

		uint32_t reflectionCheck = 0;
		std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setLayouts;
		for (auto&& [stage, data] : m_SourceCodes)
		{
			SpvReflectShaderModule module{};
			SpvReflectResult result = spvReflectCreateShaderModule(sizeof(uint32_t) * data.size(), data.data(), &module);
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "ReflectionModule creation failed!");
			
			uint32_t count = 0;
			result = spvReflectEnumerateInputVariables(&module, &count, nullptr);
			PX_CORE_INFO("Enumerated '{0}' InputVars", count);
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "InputVariable enumeration failed!");

			std::vector<SpvReflectInterfaceVariable*> inputVars(count);
			result = spvReflectEnumerateInputVariables(&module, &count, inputVars.data());
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "InputVariable querying failed!");

			count = 0;
			result = spvReflectEnumerateOutputVariables(&module, &count, nullptr);
			PX_CORE_INFO("Enumerated '{0}' OutputVars", count);
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "OutputVariable enumeration failed!");

			std::vector<SpvReflectInterfaceVariable*> outputVars(count);
			result = spvReflectEnumerateOutputVariables(&module, &count, outputVars.data());
			PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "OutputVariable querying failed!");

			
			if (stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
			{
				PX_CORE_WARN("Reflecting Vertex Stage");


				VkVertexInputBindingDescription bindingDescription{};
				bindingDescription.binding = 0;
				bindingDescription.stride = 0;
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				m_VertexInputDescription.Bindings.push_back(bindingDescription);
								
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
				//later compare to the actual mesh-information

				//Debug Print Input and outputs
#ifdef PX_DEBUG
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
#endif

				//DesciptorSets
				count = 0;
				result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
				PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "DescriptorSets enumeration failed!");

				PX_CORE_WARN("Descriptor Set count: '{0}'", count);
				std::vector<SpvReflectDescriptorSet*> reflSets(count);
				result = spvReflectEnumerateDescriptorSets(&module, &count, reflSets.data());
				PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "DescriptorSets querying failed!");

				
				struct DescriptorSetLayoutData {
					uint32_t SetNumber = 0;
					VkDescriptorSetLayoutCreateInfo CreateInfo{};
					std::vector<VkDescriptorSetLayoutBinding> Bindings;
				};

				std::vector<DescriptorSetLayoutData> setLayouts(count);
				for (size_t i = 0; i < reflSets.size(); i++)
				{
					const SpvReflectDescriptorSet& reflSet = *(reflSets[i]);
					DescriptorSetLayoutData& layoutData = setLayouts[i];
					layoutData.Bindings.resize(reflSet.binding_count);

					for (size_t j = 0; j < reflSet.binding_count; j++)
					{
						const SpvReflectDescriptorBinding& reflBinding = *(reflSet.bindings[j]);
						
						VkDescriptorSetLayoutBinding& binding = layoutData.Bindings[j];
						binding.binding = reflBinding.binding;
						binding.descriptorType = static_cast<VkDescriptorType>(reflBinding.descriptor_type);
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
					}
					layoutData.SetNumber = reflSet.set;
					layoutData.CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					layoutData.CreateInfo.pNext = nullptr;
					layoutData.CreateInfo.bindingCount = reflSet.binding_count;
					layoutData.CreateInfo.pBindings = layoutData.Bindings.data();
					VkDescriptorSetLayout layout;
					PX_CORE_VK_ASSERT(vkCreateDescriptorSetLayout(device, &layoutData.CreateInfo, nullptr, &layout), VK_SUCCESS, "Created descriptorSetLayout!");
					m_DescriptorSetLayouts.push_back(layout);

					PX_CORE_INFO("Set number: '{0}'", layoutData.SetNumber);
				}

				//Debug print Descriptors
#ifdef PX_DEBUG
				for (size_t i_sets = 0; i_sets < reflSets.size(); i_sets++)
				{
					SpvReflectDescriptorSet* reflSet = reflSets[i_sets];

					PX_CORE_INFO("Index: '{0}'", i_sets);
					PX_CORE_INFO("Set: {0}", reflSet->set);
					PX_CORE_INFO("BindingCount: {0}", reflSet->binding_count);

					for (uint32_t i_bindings = 0; i_bindings < reflSet->binding_count; i_bindings++)
					{
						PX_CORE_INFO("layout(binding = {0}, set = {1}) {2} {3}", reflSet->bindings[i_bindings]->binding, reflSet->bindings[i_bindings]->set,
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
#endif

			}//if-ShaderStage Vertex

			if (stage == VK_SHADER_STAGE_FRAGMENT_BIT)
			{
				PX_CORE_WARN("Reflecting Fragment Stage");


				count = 0;
				result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
				PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "DescriptorSets enumeration failed!");

				PX_CORE_WARN("Descriptor Set count: '{0}'", count);
				std::vector<SpvReflectDescriptorSet*> reflSets(count);
				result = spvReflectEnumerateDescriptorSets(&module, &count, reflSets.data());
				PX_CORE_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "DescriptorSets querying failed!");


				struct DescriptorSetLayoutData {
					uint32_t SetNumber = 0;
					VkDescriptorSetLayoutCreateInfo CreateInfo{};
					std::vector<VkDescriptorSetLayoutBinding> Bindings;
				};

				std::vector<DescriptorSetLayoutData> setLayouts(count);
				for (size_t i = 0; i < reflSets.size(); i++)
				{
					const SpvReflectDescriptorSet& reflSet = *(reflSets[i]);
					DescriptorSetLayoutData& layoutData = setLayouts[i];
					layoutData.Bindings.resize(reflSet.binding_count);

					for (size_t j = 0; j < reflSet.binding_count; j++)
					{
						const SpvReflectDescriptorBinding& reflBinding = *(reflSet.bindings[j]);

						VkDescriptorSetLayoutBinding& binding = layoutData.Bindings[j];
						binding.binding = reflBinding.binding;
						binding.descriptorType = static_cast<VkDescriptorType>(reflBinding.descriptor_type);
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
					}
					layoutData.SetNumber = reflSet.set;
					layoutData.CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					layoutData.CreateInfo.pNext = nullptr;
					layoutData.CreateInfo.bindingCount = reflSet.binding_count;
					layoutData.CreateInfo.pBindings = layoutData.Bindings.data();
					VkDescriptorSetLayout layout;
					PX_CORE_VK_ASSERT(vkCreateDescriptorSetLayout(device, &layoutData.CreateInfo, nullptr, &layout), VK_SUCCESS, "Created descriptorSetLayout!");
					m_DescriptorSetLayouts.push_back(layout);

					PX_CORE_INFO("Set number: '{0}'", layoutData.SetNumber);
				}

#ifdef PX_DEBUG
				for (size_t i_sets = 0; i_sets < reflSets.size(); i_sets++)
				{
					SpvReflectDescriptorSet* reflSet = reflSets[i_sets];

					PX_CORE_INFO("Index: '{0}'", i_sets);
					PX_CORE_INFO("Set: {0}", reflSet->set);
					PX_CORE_INFO("BindingCount: {0}", reflSet->binding_count);

					for (uint32_t i_bindings = 0; i_bindings < reflSet->binding_count; i_bindings++)
					{
						PX_CORE_INFO("layout(binding = {0}, set = {1}) {2} {3}", reflSet->bindings[i_bindings]->binding, reflSet->bindings[i_bindings]->set,
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
#endif

			}//if-ShaderStage Fragment


			spvReflectDestroyShaderModule(&module);
		}//for-end module


		// The layout it connected to the shader reflection. I need a way to compare different shaders and manage similarities, maybe store the layouts named? in some kind of pool?
		// I definitely need a way to link the data with the respective DescriptorSetLayout. maybe via the ShaderObject, which should be attached to the material, which is
		// attached to the object. It is also swapchain image dependent
		//size_t sceneParameterBuffersize = PadUniformBuffer(sizeof(SceneUniformBufferD), physicalProps.limits.minUniformBufferOffsetAlignment) * Application::Get()->GetSpecification().MaxFramesInFlight;
		//m_SceneParameterBuffer = VulkanBuffer::Create(sceneParameterBuffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		/*
		for (int i = 0; i < Application::Get()->GetSpecification().MaxFramesInFlight; i++)
		{
			m_Frames[i].CamUniformBuffer = VulkanBuffer::Create(sizeof(CameraUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			PX_CORE_WARN("Created Descriptor uniform buffer!");

			const int MAX_OBJECTS = 10000;
			m_Frames[i].ObjectBuffer = VulkanBuffer::Create(sizeof(GPUBufferObject) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			m_Frames[i].GlobalDescriptorSet = VulkanDescriptor::CreateSet(device, descPool, &m_GlobalDescriptorSetLayout);
			m_Frames[i].ObjectDescriptorSet = VulkanDescriptor::CreateSet(device, descPool, &m_ObjectDescriptorSetLayout);
			PX_CORE_WARN("Created Descriptor set!");

			VkDescriptorBufferInfo camInfo{};
			camInfo.buffer = m_Frames[i].CamUniformBuffer.Buffer;
			camInfo.offset = 0;
			camInfo.range = sizeof(CameraUniformBuffer);

			VkDescriptorBufferInfo sceneInfo{};
			sceneInfo.buffer = m_SceneParameterBuffer.Buffer;
			sceneInfo.offset = 0;
			sceneInfo.range = sizeof(SceneUniformBufferD);

			VkDescriptorBufferInfo objInfo{};
			objInfo.buffer = m_Frames[i].ObjectBuffer.Buffer;
			objInfo.offset = 0;
			objInfo.range = sizeof(GPUBufferObject) * MAX_OBJECTS;

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_Textures["Logo"].ImageView;
			imageInfo.sampler = m_TextureSampler;

			VkWriteDescriptorSet CreateDescriptorWrites(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, VkDescriptorImageInfo* imageInfo, uint32_t dstBinding)
			{
				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.pNext = nullptr;

				write.dstBinding = dstBinding;		// again, binding in the shader
				write.dstSet = dstSet;
				write.descriptorCount = 1;		// how many elements in the array
				write.descriptorType = type;
				write.pBufferInfo = bufferInfo;
				write.pImageInfo = imageInfo;	// optional image data

				return write;
			}
			std::array<VkWriteDescriptorSet, 4> descriptorWrites
			{
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_Frames[i].GlobalDescriptorSet, &camInfo, nullptr, 0),
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_Frames[i].GlobalDescriptorSet, &sceneInfo, nullptr, 1),
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_Frames[i].ObjectDescriptorSet, &objInfo, nullptr, 0),
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Frames[i].GlobalDescriptorSet, nullptr, &imageInfo, 2)
			};

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}*/
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

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetTargetSpirv(shaderc_spirv_version_1_5);
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
		}
	}

	VkShaderModule& VulkanShader::GetModule(VkShaderStageFlagBits stage)
	{
		PX_CORE_ASSERT(!(m_Modules.find(stage) == m_Modules.end()), "Shaderstage not covered by shader '{0}'", m_DebugName);


		return m_Modules.at(stage);
	}
}

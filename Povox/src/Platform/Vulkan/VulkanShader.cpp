#include "pxpch.h"
#include "VulkanShader.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanInitializers.h"

#include "Povox/Utils/FileUtility.h"
#include "Povox/Core/Time.h"

#include <cstdint>
#include <fstream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_reflect.h>

namespace Povox {

	namespace VulkanUtils {

		static VkShaderStageFlagBits VKShaderTypeFromString(const std::string& string)
		{
			if (string == "vert")
				return VK_SHADER_STAGE_VERTEX_BIT;
			if (string == "frag")
				return VK_SHADER_STAGE_FRAGMENT_BIT;

			PX_CORE_ASSERT(false, "Shader type not found!");
		}

		static const char* VKShaderStageCachedVulkanFileExtension(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT: return ".chached_vulkan.vert";
				case VK_SHADER_STAGE_FRAGMENT_BIT: return ".chached_vulkan.frag";
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
			}

			PX_CORE_ASSERT(false, "Shaderstage not defined");
			return (shaderc_shader_kind)0;
		}
	}


	VulkanShader::VulkanShader(const std::string& filepath)
	{
		PX_PROFILE_FUNCTION();


		Utils::Shader::CreateVKCacheDirectoryIfNeeded();

		PX_CORE_WARN("Starting to load shader from '{0}'", filepath);
		std::string sources = Utils::Shader::ReadFile(filepath);
		auto shaderSourceCodeMap = PreProcess(sources);

		// Extract name from filepath
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind(".");
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Info.DebugName = filepath.substr(lastSlash, count);

		{
			Timer timer;
			CompileOrGetVulkanBinaries(shaderSourceCodeMap);
			PX_CORE_WARN("Shader compilation took {0}ms", timer.ElapsedMilliseconds());
			//Reflect();
			//PX_CORE_WARN("Shader reflection took {0}ms", timer.ElapsedMilliseconds());
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		for (auto&& [stage, data] : m_SourceCodes)
		{
			createInfo.codeSize = data.size() * sizeof(uint32_t);
			createInfo.pCode = reinterpret_cast<const uint32_t*>(data.data());
			PX_CORE_VK_ASSERT(vkCreateShaderModule(VulkanContext::GetDevice()->GetVulkanDevice(), &createInfo, nullptr, &m_Modules[stage]), VK_SUCCESS, "Failed to create shader module!");
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

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetTargetSpirv(shaderc_spirv_version_1_5);
		const bool optimize = true;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::Shader::GetVKChacheDirectory();

		auto& shaderData = m_SourceCodes;
		shaderData.clear();
		for (auto&& [stage, code] : sources)
		{
			std::filesystem::path shaderFilePath = m_Info.Path;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + VulkanUtils::VKShaderStageCachedVulkanFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open()) // happens if chache path exists
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
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(code, VulkanUtils::VKShaderStageToShaderC(stage), m_Info.Path.c_str(), options);
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

	const VkShaderModule VulkanShader::GetModule(VkShaderStageFlagBits stage) const
	{
		PX_CORE_ASSERT(!(m_Modules.find(stage) == m_Modules.end()), "Shaderstage not covered by shader '{0}'", m_Info.DebugName);

		return m_Modules.at(stage);
	}

	//Creates a DescriptorSetLayout by reflecting the shade and allocates it form the Pool in the Context
	/*void VulkanShader::Reflect()
	{
		VkDescriptorPool descPool = VulkanContext::GetDescriptorPool();
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		VkPhysicalDeviceProperties physicalProps = VulkanContext::GetDevice()->GetPhysicalDeviceProperties();

		uint32_t reflectionCheck = 0;
		std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setLayouts;
		for (auto&& [stage, data] : m_SourceCodes)
		{
			spirv_cross::Compiler compiler(std::move(data));
			spirv_cross::ShaderResources shaderRes = compiler.get_shader_resources();

			PX_CORE_TRACE("ShaderReflection:");
			//Uniforms
			for (const auto& uniform : shaderRes.uniform_buffers)
			{
				uint32_t set = compiler.get_decoration(uniform.id, spv::DecorationDescriptorSet);
				reflectionCheck++;
				uint32_t binding = compiler.get_decoration(uniform.id, spv::DecorationBinding);
				PX_CORE_TRACE("Uniform {0}, at set: '{1}' with binding {2}", uniform.name.c_str(), set, binding);

				setLayouts[set].push_back(VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage, binding));
			}

			//SSBO
			for (const auto& ssbo : shaderRes.storage_buffers)
			{
				uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
				reflectionCheck++;
				uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
				PX_CORE_TRACE("Uniform {0}, at set: '{1}' with binding {2}", ssbo.name.c_str(), set, binding);

				setLayouts[set].push_back(VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stage | VK_SHADER_STAGE_FRAGMENT_BIT, binding));
			}

			//Sampled Images
			for (const auto& sampledImages : shaderRes.sampled_images)
			{
				uint32_t set = compiler.get_decoration(sampledImages.id, spv::DecorationDescriptorSet);
				reflectionCheck++;
				uint32_t binding = compiler.get_decoration(sampledImages.id, spv::DecorationBinding);
				PX_CORE_TRACE("Uniform {0}, at set: '{1}' with binding {2}", sampledImages.name.c_str(), set, binding);

				setLayouts[set].push_back(VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, binding));
			}

			PX_CORE_ASSERT(reflectionCheck <= physicalProps.limits.maxBoundDescriptorSets, "Too many desciptor sets ({0}) allocated in shader '{1}'", reflectionCheck, m_Info.DebugName);

			//Push constants
			uint32_t id = shaderRes.push_constant_buffers[0].id;
			spirv_cross::SmallVector<spirv_cross::BufferRange> ranges = compiler.get_active_buffer_ranges(id);
			for (const auto& range : ranges)
			{
				PX_CORE_TRACE("Accessing member {0}, offset {1}, size", range.index, range.offset, range.range);
			}
		}

		//VkDescriptorSetLayoutBinding sceneBinding = VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
		//VkDescriptorSetLayoutBinding samplerLayoutBinding = VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2);

		//VkDescriptorSetLayoutBinding objBinding = VulkanInits::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

		VkDescriptorSetLayout layout;
		for (auto&& [set, binding] : setLayouts)
		{
			layout = VulkanDescriptor::CreateLayout(device, binding);
		}


		// The layout it connected to the shader reflection. I need a way to compare different shaders and manage similarities, maybe store the layouts named? in some kind of pool?
		// I definitely need a way to link the data with the respective DescriptorSetLayout. maybe via the ShaderObject, which should be attached to the material, which is
		// attached to the object. It is also swapchain image dependent
		size_t sceneParameterBuffersize = PadUniformBuffer(sizeof(SceneUniformBufferD), physicalProps.limits.minUniformBufferOffsetAlignment) * Application::Get()->GetSpecification().MaxFramesInFlight;
		m_SceneParameterBuffer = VulkanBuffer::Create(sceneParameterBuffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

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


			std::array<VkWriteDescriptorSet, 4> descriptorWrites
			{
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_Frames[i].GlobalDescriptorSet, &camInfo, nullptr, 0),
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_Frames[i].GlobalDescriptorSet, &sceneInfo, nullptr, 1),
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_Frames[i].ObjectDescriptorSet, &objInfo, nullptr, 0),
				VulkanInits::CreateDescriptorWrites(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Frames[i].GlobalDescriptorSet, nullptr, &imageInfo, 2)
			};

			vkUpdateDescriptorSets(s_Device->GetVulkanDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}*/
}

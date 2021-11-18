#include "pxpch.h"
#include "VulkanShader.h"
#include "VulkanDebug.h"
#include "VulkanInitializers.h"

#include "Povox/Utils/FileUtility.h"

#include <cstdint>
#include <fstream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>

namespace Povox {

	namespace Utils { namespace Shader {

		static VkShaderStageFlagBits VKShaderTypeFromString(const std::string& string)
		{
			if(string == "vert")
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
	} }


	//-------------------VulkanShader---------------------------
	VkShaderModule VulkanShader::Create(VkDevice logicalDevice, const std::string& filepath)
	{
		Utils::Shader::CreateVKCacheDirectoryIfNeeded();

		std::vector<uint32_t> bufferData = CompileOrGetVulkanBinaries(filepath);

		std::ifstream stream(filepath, std::ios::ate | std::ios::binary);
		bool isOpen = stream.is_open();
		PX_CORE_ASSERT(isOpen, "Failed to open file!");

		size_t fileSize = (size_t)stream.tellg();
		std::vector<char> buffer(fileSize);

		stream.seekg(0);
		stream.read(buffer.data(), fileSize);

		PX_CORE_INFO("Shadercode size: '{0}'", buffer.size());

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = bufferData.size() * sizeof(uint32_t);
		//createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
		createInfo.pCode = bufferData.data();

		VkShaderModule shaderModule;
		PX_CORE_VK_ASSERT(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule), VK_SUCCESS, "Failed to create shader module!");
		return shaderModule;
	}

	std::vector<uint32_t> VulkanShader::CompileOrGetVulkanBinaries(const std::string& filepath)
	{
		PX_CORE_INFO("Starting to load vulkan shader from '{0}'", filepath);
		std::string source = Utils::Shader::ReadFile(filepath);
		VkShaderStageFlagBits stage = Utils::Shader::VKShaderTypeFromString(Utils::GetFileExtension(filepath));

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetTargetSpirv(shaderc_spirv_version_1_5);
		const bool optimize = true;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::Shader::GetVKChacheDirectory();

		std::vector<uint32_t> shaderData;
		std::filesystem::path shaderFilePath = filepath;
		std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::Shader::VKShaderStageCachedVulkanFileExtension(stage));

		std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
		if (in.is_open())							// happens if chache path exists
		{
			in.seekg(0, std::ios::end);				// places pointer to end of file
			auto size = in.tellg();					// position of pointer equals size
			in.seekg(0, std::ios::beg);				// places pointer back to start

			auto& data = shaderData;				// gets a vector of uint32_t
			data.resize(size / sizeof(uint32_t));	// sets the size of the vector
			in.read((char*)data.data(), size);		// reads in and writes to data I guess
		}
		else
		{
			shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::Shader::VKShaderStageToShaderC(stage), filepath.c_str(), options);
			if (module.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				PX_CORE_ERROR(module.GetErrorMessage());
				PX_CORE_ASSERT(false, module.GetErrorMessage());
			}

			shaderData = std::vector<uint32_t>(module.cbegin(), module.cend());

			std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
			if (out.is_open())
			{
				auto& data = shaderData;
				out.write((char*)data.data(), data.size() * sizeof(uint32_t));
				out.flush();
				out.close();
			}
		}

		/*for (auto&& [stage, data] : shaderData)
		{
			Reflect(stage, data);
		}*/

		return shaderData;
	}

//-------------------VulkanDescriptorPool---------------------------
	VkDescriptorPool VulkanDescriptor::CreatePool(VkDevice logicalDevice)
	{
		std::vector<VkDescriptorPoolSize> poolSizes
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		poolInfo.flags = 0;
		poolInfo.maxSets = 10;
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();


		VkDescriptorPool output;
		PX_CORE_VK_ASSERT(vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &output), VK_SUCCESS, "Failed to create descriptor pool!");
		return output;
	}

	VkDescriptorSetLayout VulkanDescriptor::CreateLayout(VkDevice logicalDevice, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings)
	{		
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;

		layoutInfo.flags = 0;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		VkDescriptorSetLayout output;
		PX_CORE_VK_ASSERT(vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &output), VK_SUCCESS, "Failed to create descriptor set layout!");
		return output;
	}

	VkDescriptorSet VulkanDescriptor::CreateSet(VkDevice logicalDevice, VkDescriptorPool pool, VkDescriptorSetLayout* layout)
	{		
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layout;

		VkDescriptorSet output;
		PX_CORE_VK_ASSERT(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &output), VK_SUCCESS, "Failed to create descriptor sets!");

		return output;
	}
}

#include "VulkanShader.h"

#include <fstream>

///////////////////////////////////////////
void VulkanShader::InitShader(const char* filename, VkShaderStageFlagBits shaderStage, VkDevice logicalDevice)
{
	std::vector<char> shaderCode = ReadFile(filename);

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &m_shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	m_createInfo = {};
	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_createInfo.stage = shaderStage;
	m_createInfo.module = m_shaderModule;
	m_createInfo.pName = "main";
}

///////////////////////////////////////////
void VulkanShader::DestroyShader(VkDevice logicalDevice)
{
	vkDestroyShaderModule(logicalDevice, m_shaderModule, nullptr);
}

///////////////////////////////////////////
VkPipelineShaderStageCreateInfo& VulkanShader::GetShaderCreateInfo()
{
	//TODO: Assert here if we do not have any attributes set
	return m_createInfo;
}

///////////////////////////////////////////
VkShaderModule VulkanShader::GetShaderModule()
{
	return m_shaderModule;
}

///////////////////////////////////////////
std::vector<char> VulkanShader::ReadFile(const char* filename)
{
	//std::ios::ate (start reading at end of file) std::ios::binary(read as a binary file and avoid text transformations)
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + std::string(filename));
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

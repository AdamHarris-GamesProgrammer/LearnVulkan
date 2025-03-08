#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN

#include <vector>

class VulkanShader {
public:
	void InitShader(const char* filename, VkShaderStageFlagBits shaderStage, VkDevice logicalDevice);
	void DestroyShader(VkDevice logicalDevice);

	VkPipelineShaderStageCreateInfo& GetShaderCreateInfo();
	VkShaderModule GetShaderModule();

private:
	std::vector<char> ReadFile(const char* filename);

private:
	VkPipelineShaderStageCreateInfo m_createInfo;
	VkShaderModule m_shaderModule;
};
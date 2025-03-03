#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN

#include <vector>

class VulkanInstance
{
public:
	void CreateInstance(const char* pApplicationName);
	void DestroyDebugMessenger();
	void DestroyInstance();

	VkInstance GetInstanceObject() const;
private:
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	void OutputSupportedExtensions();
	bool ValidateInstanceExtensionSupport(const std::vector<const char*>& extensions);
	bool ValidateValidationLayersSupport();
	std::vector<const char*> GetRequiredExtensions();

private:
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
};


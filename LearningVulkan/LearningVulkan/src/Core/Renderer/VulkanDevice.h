#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN

#include <optional>
#include <vector>

///////////////////////////////////////////
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

///////////////////////////////////////////
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

///////////////////////////////////////////
class VulkanDevice
{
public:
	void InitDevice(VkInstance instance, VkSurfaceKHR surface);
	void DestroyDevice();

	VkPhysicalDevice GetPhysicalDevice() const;
	VkDevice GetLogicalDevice() const;

	VkQueue GetGraphicsQueue() const;
	VkQueue GetPresentQueue() const;

	SwapChainSupportDetails QuerySwapChainSupport();
	QueueFamilyIndices FindQueueFamiliesForPhysicalDevice();

private:
	int RateDeviceSuitability(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

private: 
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; //Implicitely destroyed when the instance is destroyed
	VkDevice m_logicalDevice;

	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkSurfaceKHR m_surface;

	const std::vector<const char*> m_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
};


#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN

#include "VulkanDevice.h"

///////////////////////////////////////////
class VulkanSwapChain {
public:
	void InitSwapChain(GLFWwindow* pWindow, VulkanDevice* pDevices, VkSurfaceKHR surface);
	void CreateImageViews(VkDevice logicalDevice);
	void DestroyImageViews(VkDevice logicalDevice);
	void DestroySwapChain(VkDevice logicalDevice);

	VkSwapchainKHR GetSwapChain();
	VkExtent2D GetExtents();
	VkFormat GetImageFormat();
	const std::vector<VkImage>& GetImages();
	const std::vector<VkImageView>& GetImageViews();

private:
	VkSurfaceFormatKHR ChooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExent(GLFWwindow* pWindow, const VkSurfaceCapabilitiesKHR& capabilities);

private:
	VkSwapchainKHR m_swapChain;
	VkExtent2D m_swapchainExtents;
	VkFormat m_swapchainImageFormat;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;

};

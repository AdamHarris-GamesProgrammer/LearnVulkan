#include "VulkanSwapChain.h"

#include <stdexcept>
#include <algorithm>

///////////////////////////////////////////
void VulkanSwapChain::InitSwapChain(GLFWwindow* pWindow, VulkanDevice* pDevices, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details = pDevices->QuerySwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainFormat(details.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(details.presentModes);
	VkExtent2D extent = ChooseSwapExent(pWindow, details.capabilities);

	uint32_t imageCount = details.capabilities.minImageCount + 1;

	if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
		imageCount = details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //What operations we will be using these images for

	QueueFamilyIndices indices = pDevices->FindQueueFamiliesForPhysicalDevice();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	//We may support graphics and presenting on the same queue family, this offers better performance so we opt for that if possible.
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //image can be used across multiple queue families
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //image is owned by one queue family at a time and ownership must be explicitely transferted.
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = details.capabilities.currentTransform; //We can apply a transform such as rotations to each image in a swap chain, we do not want one so we pass in current transform
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkDevice logicalDevice = pDevices->GetLogicalDevice();
	if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(logicalDevice, m_swapChain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, m_swapChain, &imageCount, m_swapchainImages.data());

	m_swapchainImageFormat = surfaceFormat.format;
	m_swapchainExtents = extent;
}

///////////////////////////////////////////
void VulkanSwapChain::CreateImageViews(VkDevice logicalDevice)
{
	m_swapchainImageViews.resize(m_swapchainImages.size());

	for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapchainImages[i];

		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapchainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views!");
		}

	}
}

///////////////////////////////////////////
void VulkanSwapChain::DestroyImageViews(VkDevice logicalDevice)
{
	for (auto imageView : m_swapchainImageViews) {
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}
}

///////////////////////////////////////////
void VulkanSwapChain::DestroySwapChain(VkDevice logicalDevice)
{

	vkDestroySwapchainKHR(logicalDevice, m_swapChain, nullptr);
}

///////////////////////////////////////////
VkSwapchainKHR VulkanSwapChain::GetSwapChain()
{
	return m_swapChain;
}

///////////////////////////////////////////
VkExtent2D VulkanSwapChain::GetExtents()
{
	return m_swapchainExtents;
}

///////////////////////////////////////////
VkFormat VulkanSwapChain::GetImageFormat()
{
	return m_swapchainImageFormat;
}

///////////////////////////////////////////
const std::vector<VkImage>& VulkanSwapChain::GetImages()
{
	return m_swapchainImages;
}

///////////////////////////////////////////
const std::vector<VkImageView>& VulkanSwapChain::GetImageViews()
{
	return m_swapchainImageViews;
}

///////////////////////////////////////////
VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

///////////////////////////////////////////
VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& presentMode : availablePresentModes) {
		//Similar to FIFO but when the queue is removed we remove from the front and insert at the back
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}

	//Swap chain is a queue where the program takes an image from the front when the display is refreshed. The program continues to place renderered images at the back, if the queue is full then we wait
	return VK_PRESENT_MODE_FIFO_KHR;
}

///////////////////////////////////////////
VkExtent2D VulkanSwapChain::ChooseSwapExent(GLFWwindow* pWindow, const VkSurfaceCapabilitiesKHR& capabilities)
{
	//Resolution of the swap chain. Almost equal to the resolution of the panel we are drawing to.

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	int width, height;
	glfwGetFramebufferSize(pWindow, &width, &height);

	VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	//Clamps our framebuffer size to be within acceptable bounds given our capabilities.
	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

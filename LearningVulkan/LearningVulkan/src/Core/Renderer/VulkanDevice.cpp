#include "VulkanDevice.h"

#include "VulkanValidationLayer.h"

#include <set>
#include <string>
#include <map>
#include <stdexcept>

///////////////////////////////////////////
void VulkanDevice::InitDevice(VkInstance instance, VkSurfaceKHR surface)
{
	m_surface = surface;

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	std::multimap<int, VkPhysicalDevice> candidates;
	for (const auto& device : devices) {
		int score = RateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.rbegin()->first > 0) {
		m_physicalDevice = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("Failed to find a suitable GPU!");
	}

	QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;

	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

	if (VulkanValidationLayer::IsValidationLayerEnabled()) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanValidationLayer::GetValidationLayers().size());
		createInfo.ppEnabledLayerNames = VulkanValidationLayer::GetValidationLayers().data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	//TODO: Causes  Emulation found unrecognized structure type in pProperties->pNext - this struct will be ignored error switch API_MAKE_VERSION to the non deprecated commands in CreateInstance()
	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to Create Logical Device!");
	}

	vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);
}

///////////////////////////////////////////
void VulkanDevice::DestroyDevice()
{
	vkDestroyDevice(m_logicalDevice, nullptr);
}

///////////////////////////////////////////
VkPhysicalDevice VulkanDevice::GetPhysicalDevice() const
{
	return m_physicalDevice;
}

///////////////////////////////////////////
VkDevice VulkanDevice::GetLogicalDevice() const
{
	return m_logicalDevice;
}

///////////////////////////////////////////
VkQueue VulkanDevice::GetGraphicsQueue() const
{
	return m_graphicsQueue;
}

///////////////////////////////////////////
VkQueue VulkanDevice::GetPresentQueue() const
{
	return m_presentQueue;
}

///////////////////////////////////////////
SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport()
{
	return QuerySwapChainSupport(m_physicalDevice);
}

///////////////////////////////////////////
QueueFamilyIndices VulkanDevice::FindQueueFamiliesForPhysicalDevice()
{
	return FindQueueFamilies(m_physicalDevice);
}

///////////////////////////////////////////
int VulkanDevice::RateDeviceSuitability(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 0;

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	score += deviceProperties.limits.maxImageDimension2D;

	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdaquate = false;
	if (!extensionsSupported) {
		return 0;
	}
	else {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdaquate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		score += 100;
	}

	QueueFamilyIndices indices = FindQueueFamilies(device);
	if (!indices.IsComplete()) {
		return 0;
	}

	return score;
}

///////////////////////////////////////////
bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	//If the required extensions vector is now empty then that means we support every extension needed on this device
	return requiredExtensions.empty();
}

///////////////////////////////////////////
QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &QueueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &QueueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.IsComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

///////////////////////////////////////////
SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}

	uint32_t presentCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentCount, nullptr);

	if (presentCount != 0) {
		details.presentModes.resize(presentCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentCount, details.presentModes.data());
	}

	return details;
}

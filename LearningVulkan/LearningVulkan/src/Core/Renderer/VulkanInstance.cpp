#include "VulkanInstance.h"
#include "VulkanValidationLayer.h"

#include <iostream>
#include <stdexcept>

///////////////////////////////////////////
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

///////////////////////////////////////////
void VulkanInstance::CreateInstance(const char* pApplicationName)
{
	//We need to create these information structs to tell Vulkan information about our program. 
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = pApplicationName;
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0); //variant, major, minor, patch
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = GetRequiredExtensions();
	if (!ValidateInstanceExtensionSupport(extensions)) {
		std::cerr << "Required Extensions are not supported by instance!\n";
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (VulkanValidationLayer::IsValidationLayerEnabled() && ValidateValidationLayersSupport()) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanValidationLayer::GetValidationLayers().size());
		createInfo.ppEnabledLayerNames = VulkanValidationLayer::GetValidationLayers().data();

		debugCreateInfo = {};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		//Specifies what severity of messages this callback will be notified about
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		//Specifies what type of messages this callback will be notified about
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		//Specifies the pointer to the debug callback
		debugCreateInfo.pfnUserCallback = DebugCallback;
		debugCreateInfo.pUserData = nullptr;

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo; //In order for the validation layers to work we require a instance. This means that for creating an instance we instead provide the debug create info as pNext
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Instance!");
	}

	OutputSupportedExtensions(); //TODO: Hide this behind a log graphics variable. (Also TODO: Make a command system)

	if (VulkanValidationLayer::IsValidationLayerEnabled())
	{
		if (CreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("Failed to setup debug messenger!");
		}
	}

}

void VulkanInstance::DestroyDebugMessenger()
{
	if (VulkanValidationLayer::IsValidationLayerEnabled()) {
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}
}

///////////////////////////////////////////
void VulkanInstance::DestroyInstance()
{
	vkDestroyInstance(m_instance, nullptr);
}

///////////////////////////////////////////
VkInstance VulkanInstance::GetInstanceObject() const
{
	return m_instance;
}

///////////////////////////////////////////
std::vector<const char*> VulkanInstance::GetRequiredExtensions()
{
	uint32_t extensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);

	if (VulkanValidationLayer::IsValidationLayerEnabled()) { //Allows our debugging utils to be added when we are running a development build
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

///////////////////////////////////////////
bool VulkanInstance::ValidateInstanceExtensionSupport(const std::vector<const char*>& extensions)
{
	bool bAllLayersSupported = true;

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); //Get our extension count
	std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data()); //Fill our exentions vector

	for (const auto& extensionToQuery : extensions) {
		bool bFound = false;
		for (const auto& supportedExtention : supportedExtensions) {
			if (strcmp(supportedExtention.extensionName, extensionToQuery) == 0 && !bFound) {
				bFound = true;
			}
		}

		if (!bFound) {
			bAllLayersSupported = false;
			std::cerr << "\tExtension " << extensionToQuery << " is not supported by instance\n";
		}
	}

	return bAllLayersSupported;
}

bool VulkanInstance::ValidateValidationLayersSupport()
{
	//Gets the amount of layers in our instance
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	//fills the available layers vector with the layer data
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : VulkanValidationLayer::GetValidationLayers()) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

///////////////////////////////////////////
VkResult VulkanInstance::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

///////////////////////////////////////////
void VulkanInstance::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

///////////////////////////////////////////
void VulkanInstance::OutputSupportedExtensions()
{
	//Finds all the extensions this system supports
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "Available Extensions" << std::endl;
	for (const auto& extension : extensions) {
		std::cout << "\t" << extension.extensionName << "\n";
	}
}

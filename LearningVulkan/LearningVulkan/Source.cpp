#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

//Disables validation layers when in release mode
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
	#endif

class HelloTriangleApp {
public:
	void Run() {
		InitWindow();
		InitVulkan();
		MainLoop();
		Cleanup();
	}
private:
	void InitVulkan() {
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
	}

	void CreateInstance() {
		if (enableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("Validation layers requested, but not available");
		}

		VkApplicationInfo appInfo{};
		//Lots of structs in Vulkan require you to explicitly specify what the type of the struct is
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;


		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan Instance!");
		}

		OutputSupportedExtensions();
	}

	void OutputSupportedExtensions() {
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

	bool CheckValidationLayerSupport() {
		//Gets the amount of layers in our instance
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		//fills the available layers vector with the layer data
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
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

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		//Specifies what severity of messages this callback will be notified about
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		//Specifies what type of messages this callback will be notified about
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		//Specifies the pointer to the debug callback
		createInfo.pfnUserCallback = DebugCallback;
		createInfo.pUserData = nullptr;
	}

	void SetupDebugMessenger() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		PopulateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("Failed to setup debug messenger!");
		}

	}

	std::vector<const char*> GetRequiredExtensions() {
		uint32_t extensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

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
	
	void PickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("Failed to find GPUs with Vulkan support");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

		std::multimap<int, VkPhysicalDevice> candidates;
		for (const auto& device : devices) {
			int score = RateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		if (candidates.rbegin()->first > 0) {
			_physicalDevice = candidates.rbegin()->second;
		}
		else {
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
	}

	int RateDeviceSuitability(VkPhysicalDevice device) {
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

	VkSurfaceFormatKHR ChooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		return availableFormats[0];
	}

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& presentMode : availablePresentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return presentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(_pWindow, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void CreateSwapChain() {
		SwapChainSupportDetails details = QuerySwapChainSupport(_physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainFormat(details.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(details.presentModes);
		VkExtent2D extent = ChooseSwapExent(details.capabilities);

		uint32_t imageCount = details.capabilities.minImageCount + 1;

		if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
			imageCount = details.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = _surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = details.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swap chain!");
		}
	}

	void CreateImageViews() {
		_swapchainImageViews.resize(_swapchainImages.size());

		for (size_t i = 0; i < _swapchainImageViews.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = _swapchainImages[i];

			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = _swapchainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(_device, &createInfo, nullptr, &_swapchainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image views!");
			}

		}
	}

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		//If the required extensions vector is now empty then that means we support every extension needed on this device
		return requiredExtensions.empty();

	}

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool IsComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
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
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);

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

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
		}

		uint32_t presentCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentCount, nullptr);

		if (presentCount != 0) {
			details.presentModes.resize(presentCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentCount, details.presentModes.data());
		}

		return details;
	}

	void CreateLogicalDevice() {
		QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice);

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

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		//TODO: Causes  Emulation found unrecognized structure type in pProperties->pNext - this struct will be ignored error switch API_MAKE_VERSION to the non deprecated commands in CreateInstance()
		if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to Create Logical Device!");
		}

		vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
		vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
	}

	void CreateSurface() {
		if (glfwCreateWindowSurface(_instance, _pWindow, nullptr, &_surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	void InitWindow() {
		glfwInit();

		//Do not create a OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		//Non resizeable window for now
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		//Width, Height, Title, Specify a monitor to open on, last paramater only relevant to OpenGL
		_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);


	}

	void MainLoop() {
		//Inifnitely Loops
		while (!glfwWindowShouldClose(_pWindow)) {
			glfwPollEvents();
		}
	}

	void Cleanup() {
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
		}

		for (auto imageView : _swapchainImageViews) {
			vkDestroyImageView(_device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(_device, _swapChain, nullptr);

		vkDestroyDevice(_device, nullptr);
		
		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vkDestroyInstance(_instance, nullptr);

		//Cleanup GLFW
		glfwDestroyWindow(_pWindow);
		glfwTerminate();
	}

private:
	//GLFW
	GLFWwindow* _pWindow = nullptr;

	//Vulkan
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;

	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE; //Implicitely destroyed when the instance is destroyed
	VkDevice _device;

	VkQueue _graphicsQueue;
	VkQueue _presentQueue;

	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapChain;

	VkFormat _swapchainImageFormat;
	std::vector<VkImageView> _swapchainImageViews;
	std::vector<VkImage> _swapchainImages;
};

int main() {
	HelloTriangleApp app;

	try {
		app.Run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
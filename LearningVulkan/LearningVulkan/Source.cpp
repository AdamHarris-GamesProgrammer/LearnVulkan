#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
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
		InitVulkan();
		InitWindow();
		MainLoop();
		Cleanup();
	}
private:
	void InitVulkan() {
		CreateInstance();
		SetupDebugMessenger();
	}

	void CreateInstance() {
		if (enableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("Validation layers requested, but not available");
		}

		VkApplicationInfo appInfo{};
		//Lots of structs in Vulkan require you to explicitly specify what the type of the struct is
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;


		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(extensions.size());
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

		//Finds all the extensions this system supports
		//uint32_t extensionCount = 0;
		//vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		//
		//std::vector<VkExtensionProperties> extensions(extensionCount);
		//vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		//
		//std::cout << "Available Extensions" << std::endl;
		//for (const auto& extension : extensions) {
		//	std::cout << "\t" << extension.extensionName << "\n";
		//}
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
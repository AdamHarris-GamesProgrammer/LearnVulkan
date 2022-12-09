#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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
	}

	void CreateInstance() {
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

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		//Fills the glfwExtensions array with the amount of extensions glfw can load for it
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> requiredExtensions;
		for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
			requiredExtensions.emplace_back(glfwExtensions[i]);
		}

		requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		createInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		createInfo.enabledLayerCount = 0;
		
		if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan Instance!");
		}

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
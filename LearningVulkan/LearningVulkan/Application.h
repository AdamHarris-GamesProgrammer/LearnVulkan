#pragma once
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <vector>
#include <string>
#include <optional>

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class Application
{
public:

	void Init(const int width, const int height, const char* appName);

	void Run();

	void Cleanup();

private:
	//Helpers
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	static std::vector<char> ReadFile(const std::string& filename);
	std::vector<const char*> GetRequiredExtensions();
	bool CheckValidationLayerSupport();
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void OutputSupportedExtensions();
	int RateDeviceSuitability(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExent(const VkSurfaceCapabilitiesKHR& capabilities);

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	//VK Objects
	void CreateInstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateCommandBuffer();
	void CreateCommandPool();
	void CreateSyncObjects();

	void DrawFrame();

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

	VkRenderPass _renderPass;
	VkPipeline _graphicsPipeline;
	VkPipelineLayout _pipelineLayout;

	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapChain;
	VkExtent2D _swapchainExtents;
	VkFormat _swapchainImageFormat;
	std::vector<VkImageView> _swapchainImageViews;
	std::vector<VkImage> _swapchainImages;

	std::vector<VkFramebuffer> _swapchainFramebuffers;

	//Manages the memory that is used to store the buffers and command buffers allocated from them
	VkCommandPool _commandPool;

	VkCommandBuffer _commandBuffer;

	VkSemaphore _imageAvailableSemaphore;
	VkSemaphore _renderFinishedSemaphore;
	VkFence _inFlightFence;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};


};


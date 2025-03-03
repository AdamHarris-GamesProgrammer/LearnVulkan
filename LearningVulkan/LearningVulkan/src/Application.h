#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN

#include "Core/Renderer/VulkanInstance.h"

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
	static std::vector<char> ReadFile(const std::string& filename);
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
	//Application data
	const char* m_pApplicationName;
	int m_screenWidth;
	int m_screenHeight;
	//~Application data

	//GLFW
	GLFWwindow* m_pWindow = nullptr;
	//~GLFW

	//Abstracted Vulkan
	VulkanInstance m_vulkanInstance;
	//~Abstracted Vulkan

	//Raw Vulkan
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; //Implicitely destroyed when the instance is destroyed
	VkDevice m_device;

	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkRenderPass m_renderPass;
	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;

	VkSurfaceKHR m_surface;

	VkSwapchainKHR m_swapChain;
	VkExtent2D m_swapchainExtents;
	VkFormat m_swapchainImageFormat;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkImage> m_swapchainImages;

	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	//Manages the memory that is used to store the buffers and command buffers allocated from them
	VkCommandPool m_commandPool;

	VkCommandBuffer m_commandBuffer;

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkFence m_inFlightFence;

	const std::vector<const char*> m_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	//~Vulkan


};


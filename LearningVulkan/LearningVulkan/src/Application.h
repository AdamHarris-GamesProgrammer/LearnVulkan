#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN

#include "Core/Renderer/VulkanDevice.h"
#include "Core/Renderer/VulkanInstance.h"
#include "Core/Renderer/VulkanSwapChain.h"

#include <vector>
#include <string>
#include <optional>

///////////////////////////////////////////
class Application
{
public:

	void Init(const int width, const int height, const char* appName);

	void Run();

	void Cleanup();

private:
	//Helpers
	static std::vector<char> ReadFile(const std::string& filename);

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	//VK Objects
	void CreateSurface();
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
	VulkanDevice m_vulkanDevices;
	VulkanSwapChain m_vulkanSwapchain;
	//~Abstracted Vulkan

	//Raw Vulkan
	VkRenderPass m_renderPass;
	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;

	VkSurfaceKHR m_surface;

	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	//Manages the memory that is used to store the buffers and command buffers allocated from them
	VkCommandPool m_commandPool;

	VkCommandBuffer m_commandBuffer;

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkFence m_inFlightFence;
	//~Vulkan
};

/*
* Logging System Notes
* - For logging we need various levels of severity we will have these. Fatal, error, warning, info, debug, trace
* - These will all take in a voradic argument allowing us to pass in a bunch of params.
* - Vulkan validation layer debugging should be expanded to fully make use of this
* - We should have different colours for each level of severity.
* - We should be able to output our logs to a file.
* - Timestamps of the logs would be useful.
* - We should have some form of queue system so that when we have multiple threads trying to log we do not run into errors logging to a file
* Assertion system
* - We should have various forms of asserts for if something is a irredeemable assert or something we can note but ignore
* - All runtime errors should be replaced with fatal asserts
* - Asserts should breakpoint on the line that caused it.
* 
* General Vulkan Abstraction
* - Framebuffer class
* - Pipeline class
* - Shader class
* - Command buffer/pool class
* - Cache the queue families for the physical device in VulkanDevice
* - Some form of global environment class with access to the renderer and general purpose things like the device.
* - GLFW window class abstraction
*/


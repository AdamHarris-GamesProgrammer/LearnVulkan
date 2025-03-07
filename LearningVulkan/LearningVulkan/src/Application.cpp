#include "Application.h"

#include "Core/Renderer/VulkanValidationLayer.h"

#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>

///////////////////////////////////////////
void Application::Init(const int width, const int height, const char* appName)
{
	m_screenWidth = width;
	m_screenHeight = height;
	m_pApplicationName = appName;

	//Initialize GLFW and our window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Do not create a OpenGL context (Not needed for Vulkan)
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //TODO: Make the window resizeable
	m_pWindow = glfwCreateWindow(m_screenWidth, m_screenHeight, m_pApplicationName, nullptr, nullptr); //Last paramater only relevant to OpenGL

	//Initialize Vulkan objects
	m_vulkanInstance.CreateInstance(m_pApplicationName);
	//TODO: Seperate debug messenger from instance class? 
	CreateSurface();
	m_vulkanDevices.InitDevice(m_vulkanInstance.GetInstanceObject(), m_surface);
	m_vulkanSwapchain.InitSwapChain(m_pWindow, &m_vulkanDevices, m_surface);
	m_vulkanSwapchain.CreateImageViews(m_vulkanDevices.GetLogicalDevice());
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffer();
	CreateSyncObjects();
}

///////////////////////////////////////////
void Application::Run()
{
	while (!glfwWindowShouldClose(m_pWindow)) {
		glfwPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(m_vulkanDevices.GetLogicalDevice());
}

///////////////////////////////////////////
void Application::Cleanup()
{
	m_vulkanInstance.DestroyDebugMessenger();

	VkDevice logicalDevice = m_vulkanDevices.GetLogicalDevice();
	for (auto fbs : m_swapchainFramebuffers) {
		vkDestroyFramebuffer(logicalDevice, fbs, nullptr);
	}

	m_vulkanSwapchain.DestroyImageViews(logicalDevice);

	vkDestroySemaphore(logicalDevice, m_imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(logicalDevice, m_renderFinishedSemaphore, nullptr);
	vkDestroyFence(logicalDevice, m_inFlightFence, nullptr);

	vkDestroyPipeline(logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(logicalDevice, m_renderPass, nullptr);

	m_vulkanSwapchain.DestroySwapChain(logicalDevice);

	vkDestroyCommandPool(logicalDevice, m_commandPool, nullptr);

	m_vulkanDevices.DestroyDevice();

	vkDestroySurfaceKHR(m_vulkanInstance.GetInstanceObject(), m_surface, nullptr);

	m_vulkanInstance.DestroyInstance();

	//Cleanup GLFW
	glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}

///////////////////////////////////////////
std::vector<char> Application::ReadFile(const std::string& filename)
{
	//std::ios::ate (start reading at end of file) std::ios::binary(read as a binary file and avoid text transformations)
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

///////////////////////////////////////////
void Application::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(m_commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Record Command Buffer");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	//Define what attachements to bind
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];

	//Define the size of the render pass area
	VkExtent2D swapchainExtents = m_vulkanSwapchain.GetExtents();
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = swapchainExtents;

	//Defins the clear color
	VkClearValue clearColor = { {{0.0f,0.0f,0.0f,1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	//Create and set the viewport
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtents.width);
	viewport.height = static_cast<float>(swapchainExtents.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

	//Create and set the scissor
	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = swapchainExtents;
	vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);

	//command buffer, vertex count, instance count, first vertex, first instance
	vkCmdDraw(m_commandBuffer, 3, 1, 0, 0);

	//Finish our render pass
	vkCmdEndRenderPass(m_commandBuffer);

	//End the command buffer and check for success
	if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer!");
	}
}

///////////////////////////////////////////
VkShaderModule Application::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_vulkanDevices.GetLogicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	return shaderModule;
}

///////////////////////////////////////////
void Application::CreateSurface()
{
	if (glfwCreateWindowSurface(m_vulkanInstance.GetInstanceObject(), m_pWindow, nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}
}

///////////////////////////////////////////
void Application::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_vulkanSwapchain.GetImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; //Directly read from the shader
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependancy{};
	dependancy.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependancy.dstSubpass = 0;

	dependancy.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependancy.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependancy;

	if (vkCreateRenderPass(m_vulkanDevices.GetLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Render Pass!");
	}
}

///////////////////////////////////////////
void Application::CreateGraphicsPipeline()
{
	auto vertShaderCode = ReadFile("shaders/vert.spv");
	auto fragShaderCode = ReadFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkExtent2D swapchainExtents = m_vulkanSwapchain.GetExtents();
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapchainExtents.width;
	viewport.height = (float)swapchainExtents.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtents;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_vulkanDevices.GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	VkDevice logicalDevice = m_vulkanDevices.GetLogicalDevice();
	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

///////////////////////////////////////////
void Application::CreateFramebuffers()
{
	m_swapchainFramebuffers.resize(m_vulkanSwapchain.GetImageViews().size());

	for (size_t i = 0; i < m_vulkanSwapchain.GetImageViews().size(); ++i) {
		VkImageView attachment[] = {
			m_vulkanSwapchain.GetImageViews()[i]
		};

		VkExtent2D extents = m_vulkanSwapchain.GetExtents();
		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachment;
		createInfo.width = extents.width;
		createInfo.height = extents.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(m_vulkanDevices.GetLogicalDevice(), &createInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create a framebuffer!");
		}
	}
}

///////////////////////////////////////////
void Application::CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	//Specifies that this command can be submitted to a queue for execution but cannot be called from other command buffers
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(m_vulkanDevices.GetLogicalDevice(), &allocInfo, &m_commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Allocate Command Buffer");
	}
}

///////////////////////////////////////////
void Application::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = m_vulkanDevices.FindQueueFamiliesForPhysicalDevice();

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //Allows our command buffer to be rerecorded individually
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(m_vulkanDevices.GetLogicalDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create commnad pool!");
	}
}

///////////////////////////////////////////
void Application::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Creates the fence in a already signaled state

	VkDevice logicalDevice = m_vulkanDevices.GetLogicalDevice();
	if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(logicalDevice, &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Sychronisation objects!");
	}
}

///////////////////////////////////////////
void Application::DrawFrame()
{
	//Wait for previous frame to finish
	VkDevice logicalDevice = m_vulkanDevices.GetLogicalDevice();
	vkWaitForFences(logicalDevice, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(logicalDevice, 1, &m_inFlightFence);

	//acquire an image from swap chain
	uint32_t imageIndex;
	vkAcquireNextImageKHR(logicalDevice, m_vulkanSwapchain.GetSwapChain(), UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	//Record a command buffer which draws the scene
	vkResetCommandBuffer(m_commandBuffer, 0);
	RecordCommandBuffer(m_commandBuffer, imageIndex);

	//Submit the recorded command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemophores[] = { m_imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemophores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_vulkanDevices.GetGraphicsQueue(), 1, &submitInfo, m_inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}


	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_vulkanSwapchain.GetSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(m_vulkanDevices.GetPresentQueue(), &presentInfo);


	//Present the swap chain image
}

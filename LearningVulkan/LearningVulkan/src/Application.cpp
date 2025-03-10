#include "Application.h"
#include "Core/Logger.h"

#include "Core/Renderer/VulkanValidationLayer.h"


#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>

const int MAX_FRAMES_IN_FLIGHT = 2;

///////////////////////////////////////////
void Application::Init(const int width, const int height, const char* appName)
{
	m_screenWidth = width;
	m_screenHeight = height;
	m_pApplicationName = appName;

	InitializeLogger();
	
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
	m_vulkanVertexShader.InitShader("shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT, m_vulkanDevices.GetLogicalDevice());
	m_vulkanFragmentShader.InitShader("shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, m_vulkanDevices.GetLogicalDevice());
	CreateGraphicsPipeline();
	m_vulkanSwapchain.CreateFramebuffers(m_vulkanDevices.GetLogicalDevice(), m_renderPass);
	CreateCommandPool();
	CreateCommandBuffers(); //auto-destroyed when the pool is, we do not need to explicitely destroy the command buffers
	CreateSyncObjects();
}

///////////////////////////////////////////
void Application::Run()
{
	while (!glfwWindowShouldClose(m_pWindow)) {
		glfwPollEvents();
		DrawFrame();
	}

	//ensures we keep going until drawing and presentation operations are finished
	vkDeviceWaitIdle(m_vulkanDevices.GetLogicalDevice());
}

///////////////////////////////////////////
void Application::Cleanup()
{
	m_vulkanInstance.DestroyDebugMessenger();

	VkDevice logicalDevice = m_vulkanDevices.GetLogicalDevice();
	m_vulkanSwapchain.DestroyFramebuffers(logicalDevice);
	m_vulkanSwapchain.DestroyImageViews(logicalDevice);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(logicalDevice, m_imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(logicalDevice, m_renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(logicalDevice, m_inFlightFences[i], nullptr);
	}

	vkDestroyPipeline(logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(logicalDevice, m_renderPass, nullptr);

	m_vulkanSwapchain.DestroySwapChain(logicalDevice);

	vkDestroyCommandPool(logicalDevice, m_commandPool, nullptr);

	m_vulkanFragmentShader.DestroyShader(logicalDevice);
	m_vulkanVertexShader.DestroyShader(logicalDevice);

	m_vulkanDevices.DestroyDevice();

	vkDestroySurfaceKHR(m_vulkanInstance.GetInstanceObject(), m_surface, nullptr);

	m_vulkanInstance.DestroyInstance();

	//Cleanup GLFW
	glfwDestroyWindow(m_pWindow);
	glfwTerminate();

	ShutdownLogger();
}

///////////////////////////////////////////
void Application::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	//This function writes commands that we want to execute to the command buffer.
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; //optional. Either ONE_TIME_SUBMIT, RENDER_PASS_CONTINUE or SIMULTANEOUS_USE (cmd buffer will be rerecorded after execution, secondary cmd buffer that will be in one render pass, cmd buffer can be resubmitted)
	beginInfo.pInheritanceInfo = nullptr; //optional (only relevent for secondary buffers)

	//Begin our recording
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Record Command Buffer");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	//Define what attachements to bind
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_vulkanSwapchain.GetFramebuffers()[imageIndex];
	//We need to bind the framebuffer for the swapchain that we want to draw to. 

	//Define the size of the render pass area
	VkExtent2D swapchainExtents = m_vulkanSwapchain.GetExtents();
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = swapchainExtents;

	//Defins the clear color
	VkClearValue clearColor = { {{0.0f,0.0f,0.0f,1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	//Render pass can now begin. 
	//SUBPASS_CONTENTS_INLINE means that no secondary cmd buffer will be executed
	//CONTENTS_SECONDARY_COMMAND_BUFFERS mean that the render pass commands will be executed from secondary command buffers
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//2nd param states if this is a graphics or compute pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	//Create and set the viewport (Area we want to render to in this cmd)
	VkViewport viewport = m_vulkanSwapchain.GetViewport();
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	//Create and set the scissor
	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = swapchainExtents;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	//command buffer, vertex count, instance count, first vertex, first instance
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	//Finish our render pass
	vkCmdEndRenderPass(commandBuffer);

	//End the command buffer and check for success
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer!");
	}
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
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //what should we do with this data. Options load, clear, don't care
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //store or don't care. Contents will be saved in memory for later use
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; //Directly read from the shader
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; //images are used as color attachment

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; //Explicitly declare this as a graphics pass
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependancy{};
	dependancy.srcSubpass = VK_SUBPASS_EXTERNAL; //implicit before and after subpasses
	dependancy.dstSubpass = 0;

	//The operations this subpass will wait on. We need to wait on the swap chain to finish reading from the image. This is accomplished by waiting on the color attachment stage
	dependancy.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependancy.srcAccessMask = 0;
	
	//The operations that will wait on the above requirements are writing to the color attachment. 
	dependancy.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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
	VkPipelineShaderStageCreateInfo shaderStages[] = { m_vulkanVertexShader.GetShaderCreateInfo(), m_vulkanFragmentShader.GetShaderCreateInfo()};

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
	vertexInputInfo.pVertexBindingDescriptions = nullptr; //optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; //optional

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
	rasterizer.depthClampEnable = VK_FALSE; //useful for shadow mapping
	rasterizer.rasterizerDiscardEnable = VK_FALSE; //geometry will never pass through raster stage
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //fill, line, or point
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

	//configure per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; //optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; //optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; //optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; //optional

	//global colour blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; //optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; //optional
	colorBlending.blendConstants[1] = 0.0f; //optional
	colorBlending.blendConstants[2] = 0.0f; //optional
	colorBlending.blendConstants[3] = 0.0f; //optional

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; //optional
	pipelineLayoutInfo.pSetLayouts = nullptr; //optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; //optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; //optional

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
	pipelineInfo.pDepthStencilState = nullptr; //optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;

	pipelineInfo.layout = m_pipelineLayout;

	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0; //what index of the sub passes should this pipeline be used

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; //vulkan allows you to create a base pipeline that you can then extend by setting different attributes. Switching between shared pipelines is also faster
	pipelineInfo.basePipelineIndex = -1;

	VkDevice logicalDevice = m_vulkanDevices.GetLogicalDevice();
	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline!");
	}
}

///////////////////////////////////////////
void Application::CreateCommandBuffers()
{
	m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	//LEVEL_SECONDARY means we cannot submit directly but can be called from primary command buffers
	//LEVEL_PRIMARY means it can be submitted directly but cannot be called from other command buffers
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) m_commandBuffers.size();

	if (vkAllocateCommandBuffers(m_vulkanDevices.GetLogicalDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Allocate Command Buffer");
	}
}

///////////////////////////////////////////
void Application::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = m_vulkanDevices.FindQueueFamiliesForPhysicalDevice();

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	//Either RESET_COMMAND_BUFFER_BIT or CREATE_TRANSIENT_BIT
	//Reset bit allows for commands to be re recorded invidividually, without this whenever we change a command it would have to reset the whole pool
	//Transient bit allows for constant modification to the commands, this can change memory allocation behaviour.
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //Allows our command buffer to be rerecorded individually
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(m_vulkanDevices.GetLogicalDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create commnad pool!");
	}
}

///////////////////////////////////////////
void Application::CreateSyncObjects()
{
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Creates the fence in a already signaled state

	VkDevice logicalDevice = m_vulkanDevices.GetLogicalDevice();
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Sychronisation objects!");
		}
	}
}

///////////////////////////////////////////
void Application::DrawFrame()
{
	/* Outline of frame
	- Wait for previous frame to finish
	- Acquire an image from the swap chain
	- Record the cmd buffer
	- Submit the recorded command buffer
	- Present the swap cahin image
	*/

	/* Synchonrization
	- Vulkan is designed to be fully aynchronous however for drawing a frame we have a certain order that need to ensure happens in the correct order
	- We use Semaphores and Fences for this. 
	   - Semaphore: Used to add order between queue operations. Queue operations refer to work we submit to a queue.
	      - Only pauses the GPU, does not pause the CPU. 
		  - Semaphores are provided as a signal in one function and as a wait in another. 
	   - Fence: Used for ordering sycnhronisation on the CPU side. 
		- Only pauses the CPU, does not pause the GPU

	- Summary. Semaphores are used to keep execution in order on the GPU. Fences are used to keep the GPU and CPU in tandom
	*/

	//Wait for previous frame to finish. (Pause the CPU) 
	VkDevice logicalDevice = m_vulkanDevices.GetLogicalDevice();
	vkWaitForFences(logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX); //Wait for our previous frame to finish. i.e wait for the in flight fence to be signalled
	vkResetFences(logicalDevice, 1, &m_inFlightFences[m_currentFrame]); //Fences have to be manually reset. Unsignal our fence

	//acquire an image from swap chain
	uint32_t imageIndex;
	//Use the image available semaphore as a signal. I.e. we cannot execute any tasks that are waiting on this semaphore until it "signals" that is done
	//UINT64_MAX: timeout for this operation, semaphore: semaphore we will signal when we have acquired the image. NULL: optional fence, we can use fence or semaphore or both
	vkAcquireNextImageKHR(logicalDevice, m_vulkanSwapchain.GetSwapChain(), UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	//Reset our cmd buffer and record it. 
	vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
	RecordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

	//Submit the recorded command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemophores[] = { m_imageAvailableSemaphores[m_currentFrame] }; //Here we specify our image available semaphore as a signal we need to wait for. We do not submit the cmd buffer until we have acquired an image.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; //the stage in the pipeline that we should wait on
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemophores;
	submitInfo.pWaitDstStageMask = waitStages;

	//Our command buffer(s) we want to submit for execution
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

	//We pass in the render finished semaphore as our singal object that will be signalled once we have finished executing our cmd buffer.
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	//By using our fence here we now know when it is safe to re-use our command buffer
	if (vkQueueSubmit(m_vulkanDevices.GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	//Presentation stage
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores; //we pass our render finished semaphore here to prevent immediate execution.

	VkSwapchainKHR swapChains[] = { m_vulkanSwapchain.GetSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr; //optional. Allows an array of VkResults to ensure that every swapchain is successful

	vkQueuePresentKHR(m_vulkanDevices.GetPresentQueue(), &presentInfo);

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

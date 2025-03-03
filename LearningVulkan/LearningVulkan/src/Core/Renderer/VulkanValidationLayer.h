#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN

#include <vector>

static class VulkanValidationLayer
{
public:
	static bool IsValidationLayerEnabled();
	static const std::vector<const char*>& GetValidationLayers();

private:
	static const bool m_bEnableValidationLayers;
	static const std::vector<const char*> m_validationLayers;

};


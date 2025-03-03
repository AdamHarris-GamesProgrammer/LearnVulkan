#include "VulkanValidationLayer.h"

#ifdef NDEBUG
const bool VulkanValidationLayer::m_bEnableValidationLayers = false;
#else
const bool VulkanValidationLayer::m_bEnableValidationLayers = true;
#endif

const std::vector<const char*> VulkanValidationLayer::m_validationLayers = {
		"VK_LAYER_KHRONOS_validation"
};

///////////////////////////////////////////
bool VulkanValidationLayer::IsValidationLayerEnabled()
{
    return m_bEnableValidationLayers;
}

///////////////////////////////////////////
const std::vector<const char*>& VulkanValidationLayer::GetValidationLayers()
{
	return m_validationLayers;
}

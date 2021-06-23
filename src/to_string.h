#ifndef TO_STRING_H 
#define TO_STRING_H 

#include <string>
#include "vulkan/vulkan.h"

//NOTE: following functions from the official Vulkan Samples:

const std::string vulkanToString(VkFormat format);
const std::string vulkanToString(VkPresentModeKHR presentMode);
const std::string vulkanToString(VkResult result);
const std::string vulkanToString(VkSurfaceTransformFlagBitsKHR transformFlag);
const std::string vulkanToString(VkSurfaceFormatKHR surfaceFormat);
const std::string vulkanToString(VkCompositeAlphaFlagBitsKHR compositeAlpha);
const std::string vulkanToString(VkImageUsageFlagBits imageUsage);
const std::string vulkanToString(VkExtent2D extent);
const std::string vulkanToString(VkSampleCountFlagBits flagBits);
const std::string vulkanToString(VkPhysicalDeviceType type);
const std::string vulkanToString(VkImageTiling tiling);
const std::string vulkanToString(VkImageType tiling);
const std::string vulkanToString(VkBlendFactor blend);
const std::string vulkanToString(VkVertexInputRate rate);
const std::string vulkanToString(VkBool32 state);
const std::string vulkanToString(VkPrimitiveTopology topology);
const std::string vulkanToString(VkFrontFace face);
const std::string vulkanToString(VkPolygonMode mode);
const std::string vulkanToString(VkCompareOp operation);
const std::string vulkanToString(VkStencilOp operation);
const std::string vulkanToString(VkLogicOp operation);
const std::string vulkanToString(VkBlendOp operation);

#endif
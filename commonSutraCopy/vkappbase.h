#pragma once
#define WIN32_LEAD_AND_MEAN
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_win32.h>

#include <vector>

class VulkanAppBase
{
public:
	VulkanAppBase();
	virtual ~VulkanAppBase() {}
	void intialize(GLFWwindow* window, const char* appName);
	void terminate();
	virtual void prepare();
	virtual void render();

protected:
	static void checkResult(VkResult result);
	void initializeInstance(const char* appName);
	void selectPhysicalDevice();
	void enableDebugReport();
	void disableDebugReport();
	uint32_t searchGraphicsqueueIndex();
	void createDevice();
	void prepareCommandPool();
	void selectSurfaceFormat(VkFormat format);
	void createSwapchain(GLFWwindow* window);
	uint32_t getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const;
	void createDepthBuffer();
	void createViews();
	void createRenderPass();
	void createFramebuffer();
	void prepareCommandBuffers();

	VkInstance m_instance = nullptr;
	VkPhysicalDevice m_physDev = nullptr;
	VkPhysicalDeviceMemoryProperties m_physMemProps;
	uint32_t m_graphicsQueueIndex = ~0u;

	VkDevice m_device = nullptr;
	VkQueue m_deviceQueue = nullptr;
	VkCommandPool m_commandPool = nullptr;

	VkSurfaceKHR m_surface = nullptr;
	VkSurfaceFormatKHR m_surfaceFormat;
	VkSurfaceCapabilitiesKHR m_surfaceCaps;
	VkSwapchainKHR m_swapchain = nullptr;
	VkExtent2D m_swapchainExtent;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainViews;

	VkImage m_depthBuffer = nullptr;
	VkDeviceMemory m_depthBufferMemory = nullptr;
	VkImageView m_depthBufferView;

	VkRenderPass m_renderPass = nullptr;
	std::vector<VkFramebuffer> m_framebuffers;

	// デバッグレポート関連::デバッグレポート関
	PFN_vkCreateDebugReportCallbackEXT m_vkCreateDebugReportCallbackEXT = nullptr;
	PFN_vkDebugReportMessageEXT m_vkDebugReportMessageEXT = nullptr;
	PFN_vkDestroyDebugReportCallbackEXT m_vkDestroyDebugReportCallbackEXT = nullptr;
	VkDebugReportCallbackEXT m_debugReport = nullptr;

	std::vector<VkCommandBuffer> m_commands;
	std::vector<VkFence> m_fences;
};


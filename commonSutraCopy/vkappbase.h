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
	virtual ~VulkanAppBase() {}
	void intialize(GLFWwindow* window, const char* appName);
	void terminate();
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
	void prepareSemaphores();

	VkInstance m_instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
	VkPhysicalDeviceMemoryProperties m_physMemProps;
	uint32_t m_graphicsQueueIndex = ~0u;

	VkDevice m_device = VK_NULL_HANDLE;
	VkQueue m_deviceQueue = VK_NULL_HANDLE;
	VkCommandPool m_commandPool = VK_NULL_HANDLE;

	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSurfaceFormatKHR m_surfaceFormat;
	VkSurfaceCapabilitiesKHR m_surfaceCaps;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkExtent2D m_swapchainExtent;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainViews;

	VkImage m_depthBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_depthBufferMemory = VK_NULL_HANDLE;
	VkImageView m_depthBufferView;

	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_framebuffers;

	// デバッグレポート関連::デバッグレポート関
	PFN_vkCreateDebugReportCallbackEXT m_vkCreateDebugReportCallbackEXT = VK_NULL_HANDLE;
	PFN_vkDebugReportMessageEXT m_vkDebugReportMessageEXT = VK_NULL_HANDLE;
	PFN_vkDestroyDebugReportCallbackEXT m_vkDestroyDebugReportCallbackEXT = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT m_debugReport = VK_NULL_HANDLE;

	std::vector<VkCommandBuffer> m_commands;
	std::vector<VkFence> m_fences;
	VkSemaphore m_renderCompletedSem, m_presentCompletedSem = VK_NULL_HANDLE;
};


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
	uint32_t searchGraphicsqueueIndex();
	void createDevice();
	void prepareCommandPool();
	void selectSurfaceFormat(VkFormat format);
	void createSwapchain(GLFWwindow* window);
	uint32_t getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const;
	void createDepthBuffer();

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

	VkImage m_depthBuffer = nullptr;
	VkDeviceMemory m_depthBufferMemory = nullptr;
};


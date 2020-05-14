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
	void initializeInstance(const char* appName);
};


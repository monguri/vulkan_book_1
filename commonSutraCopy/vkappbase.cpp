#include "vkappbase.h"
#include <vector>

VulkanAppBase::VulkanAppBase()
{
}

void VulkanAppBase::intialize(GLFWwindow* window, const char* appName)
{
	initializeInstance(appName);
	selectPhysicalDevice();
	m_graphicsQueueIndex = searchGraphicsqueueIndex();
	createDevice();
	prepareCommandPool();
}

void VulkanAppBase::terminate()
{
}

void VulkanAppBase::checkResult(VkResult result)
{
	if (result != VK_SUCCESS)
	{
		DebugBreak();
	}
}

void VulkanAppBase::initializeInstance(const char* appName)
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName;
	appInfo.pEngineName = appName;
	appInfo.apiVersion = VK_API_VERSION_1_1;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	// インスタンス拡張情報の取得
	std::vector<const char*> extensions;
	std::vector<VkExtensionProperties> props;
	{
		uint32_t count = 0;
		// 数だけ先に取得して配列サイズを確保しておく
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		props.resize(count);
		// 配列に取得
		vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

		for (const VkExtensionProperties& v : props)
		{
			extensions.push_back(v.extensionName);
		}
	}

	VkInstanceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	ci.enabledExtensionCount = uint32_t(extensions.size());
	ci.ppEnabledExtensionNames = extensions.data();
	ci.pApplicationInfo = &appInfo;
#ifdef _DEBUG
	// デバッグビルドでは検証レイヤーを有効化
	const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
	ci.enabledLayerCount = 1;
	ci.ppEnabledLayerNames = layers;
#endif

	VkResult result = vkCreateInstance(&ci, nullptr, &m_instance);
	checkResult(result);
}

void VulkanAppBase::selectPhysicalDevice()
{
	uint32_t devCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &devCount, nullptr);
	std::vector<VkPhysicalDevice> physDevs(devCount);
	vkEnumeratePhysicalDevices(m_instance, &devCount, physDevs.data());

	// 最初のデバイスを使用する
	m_physDev = physDevs[0];
	// メモリプロパティを用意しておく
	vkGetPhysicalDeviceMemoryProperties(m_physDev, &m_physMemProps);
}

uint32_t VulkanAppBase::searchGraphicsqueueIndex()
{
	uint32_t propCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physDev, &propCount, nullptr);
	std::vector<VkQueueFamilyProperties> props(propCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physDev, &propCount, props.data());

	uint32_t graphicsQueue = 0;
	for (uint32_t i = 0; i < propCount; ++i)
	{
		if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			graphicsQueue = i;
			break;
		}
	}

	return graphicsQueue;
}

void VulkanAppBase::createDevice()
{
	const float defaultQueuePriority(1.0f);
	VkDeviceQueueCreateInfo devQueueCI{};
	devQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	devQueueCI.queueFamilyIndex = m_graphicsQueueIndex;
	devQueueCI.queueCount = 1;
	devQueueCI.pQueuePriorities = &defaultQueuePriority;


	// デバイス拡張情報をここでも取得
	std::vector<const char*> extensions;
	std::vector<VkExtensionProperties> devExtProps;
	{
		uint32_t count = 0;
		// 数だけ先に取得して配列サイズを確保しておく
		vkEnumerateDeviceExtensionProperties(m_physDev, nullptr, &count, nullptr);
		devExtProps.resize(count);
		// 配列に取得
		vkEnumerateDeviceExtensionProperties(m_physDev, nullptr, &count, devExtProps.data());

		for (const VkExtensionProperties& v : devExtProps)
		{
			extensions.push_back(v.extensionName);
		}
	}

	VkDeviceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	ci.pQueueCreateInfos = &devQueueCI;
	ci.queueCreateInfoCount = 1;
	ci.ppEnabledExtensionNames = extensions.data();
	ci.enabledExtensionCount = uint32_t(extensions.size());

	VkResult result = vkCreateDevice(m_physDev, &ci, nullptr, &m_device);
	checkResult(result);

	vkGetDeviceQueue(m_device, m_graphicsQueueIndex, 0, &m_deviceQueue);
}

void VulkanAppBase::prepareCommandPool()
{
	VkCommandPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	ci.queueFamilyIndex = m_graphicsQueueIndex;
	ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkResult result = vkCreateCommandPool(m_device, &ci, nullptr, &m_commandPool);
	checkResult(result);
}

void VulkanAppBase::prepare()
{
}

void VulkanAppBase::render()
{
}


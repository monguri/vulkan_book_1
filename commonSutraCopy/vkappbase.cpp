#include "vkappbase.h"
#include <algorithm>

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

	VkResult result = glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface);
	checkResult(result);

	selectSurfaceFormat(VK_FORMAT_B8G8R8A8_UNORM);

	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physDev, m_surface, &m_surfaceCaps);
	checkResult(result);

	createSwapchain(window);

	createDepthBuffer();
	createViews();
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

	// �C���X�^���X�g�����̎擾
	std::vector<const char*> extensions;
	std::vector<VkExtensionProperties> props;
	{
		uint32_t count = 0;
		// ��������Ɏ擾���Ĕz��T�C�Y���m�ۂ��Ă���
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		props.resize(count);
		// �z��Ɏ擾
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
	// �f�o�b�O�r���h�ł͌��؃��C���[��L����
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

	// �ŏ��̃f�o�C�X���g�p����
	m_physDev = physDevs[0];
	// �������v���p�e�B��p�ӂ��Ă���
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


	// �f�o�C�X�g�����������ł��擾
	std::vector<const char*> extensions;
	std::vector<VkExtensionProperties> devExtProps;
	{
		uint32_t count = 0;
		// ��������Ɏ擾���Ĕz��T�C�Y���m�ۂ��Ă���
		vkEnumerateDeviceExtensionProperties(m_physDev, nullptr, &count, nullptr);
		devExtProps.resize(count);
		// �z��Ɏ擾
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

void VulkanAppBase::selectSurfaceFormat(VkFormat format)
{
	uint32_t surfaceFormatCont = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physDev, m_surface, &surfaceFormatCont, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCont);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physDev, m_surface, &surfaceFormatCont, formats.data());

	for (VkSurfaceFormatKHR f : formats)
	{
		if (f.format == format)
		{
			m_surfaceFormat = f;
		}
	}
}

void VulkanAppBase::createSwapchain(GLFWwindow* window)
{
	uint32_t imageCount = (std::max)(2u, m_surfaceCaps.minImageCount); // windows��max�Ƃ���define�ƊԈႦ���Ȃ��悤��()�ň͂�ł���
	VkExtent2D& extent = m_surfaceCaps.currentExtent;
	if (extent.width == ~0u)
	{
		// �l�������Ȃ̂ŃE�B���h�E�T�C�Y���g�p����
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		extent.width = uint32_t(width);
		extent.height = uint32_t(height);
	}

	VkSwapchainCreateInfoKHR ci{};
	ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	ci.surface = m_surface;
	ci.minImageCount = imageCount;
	ci.imageFormat = m_surfaceFormat.format;
	ci.imageColorSpace = m_surfaceFormat.colorSpace;
	ci.imageExtent = extent;
	ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	ci.preTransform = m_surfaceCaps.currentTransform;
	ci.imageArrayLayers = 1;
	ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ci.queueFamilyIndexCount = 0;
	ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	ci.oldSwapchain = VK_NULL_HANDLE;
	ci.clipped = VK_TRUE;
	ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VkResult result = vkCreateSwapchainKHR(m_device, &ci, nullptr, &m_swapchain);
	checkResult(result);

	m_swapchainExtent = extent;
}

uint32_t VulkanAppBase::getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const
{
	uint32_t result = ~0u;

	for (uint32_t i = 0; i < m_physMemProps.memoryTypeCount; ++i)
	{
		if (requestBits & 1) // i���̃r�b�g��1�̂Ƃ�������r����
		{
			const VkMemoryType& type = m_physMemProps.memoryTypes[i];
			if ((type.propertyFlags & requestProps) == requestProps)
			{
				result = i;
				break;
			}
		}
		requestBits >>= 1;
	}

	return result;
}

void VulkanAppBase::createDepthBuffer()
{
	VkImageCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ci.imageType = VK_IMAGE_TYPE_2D;
	ci.format = VK_FORMAT_D32_SFLOAT;
	ci.extent.width = m_swapchainExtent.width;
	ci.extent.height = m_swapchainExtent.height;
	ci.extent.depth = 1;
	ci.mipLevels = 1;
	ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	ci.samples = VK_SAMPLE_COUNT_1_BIT;
	ci.arrayLayers = 1;
	VkResult result = vkCreateImage(m_device, &ci, nullptr, &m_depthBuffer);
	checkResult(result);

	VkMemoryRequirements reqs;
	vkGetImageMemoryRequirements(m_device, m_depthBuffer, &reqs);

	VkMemoryAllocateInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	ai.allocationSize = reqs.size;
	ai.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	result = vkAllocateMemory(m_device, &ai, nullptr, &m_depthBufferMemory);
	checkResult(result);
	result = vkBindImageMemory(m_device, m_depthBuffer, m_depthBufferMemory, 0);
	checkResult(result);
}

void VulkanAppBase::createViews()
{
	// �X���b�v�`�F�C���̂��J���[�o�b�t�@�p�̃r���[�쐬
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

	m_swapchainViews.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		VkImageViewCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ci.format = m_surfaceFormat.format;
		ci.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
		};
		ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		ci.image = m_swapchainImages[i];
		VkResult result = vkCreateImageView(m_device, &ci, nullptr, &m_swapchainViews[i]);
		checkResult(result);
	}

	// �f�v�X�o�b�t�@�p�̃r���[�쐬
	{
		VkImageViewCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ci.format = VK_FORMAT_D32_SFLOAT;
		ci.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
		};
		ci.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
		ci.image = m_depthBuffer;
		VkResult result = vkCreateImageView(m_device, &ci, nullptr, &m_depthBufferView);
		checkResult(result);
	}
}

void VulkanAppBase::prepare()
{
}

void VulkanAppBase::render()
{
}


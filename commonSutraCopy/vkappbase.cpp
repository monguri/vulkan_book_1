#include "vkappbase.h"
#include <sstream>
#include <algorithm>
#include <array>

#define GetInstanceProcAddr(FuncName) \
m_##FuncName = reinterpret_cast<PFN_##FuncName>(vkGetInstanceProcAddr(m_instance, #FuncName))

static VkBool32 VKAPI_CALL DebugReportCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData)
{
	VkBool32 ret = VK_FALSE;
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT || flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		ret = VK_TRUE;
	}

	std::stringstream ss;
	if (pLayerPrefix)
	{
		ss << "[" << pLayerPrefix << "] ";
	}
	ss << pMessage << std::endl;

	OutputDebugStringA(ss.str().c_str());

	return ret;

}

void VulkanAppBase::intialize(GLFWwindow* window, const char* appName)
{
	initializeInstance(appName);
	selectPhysicalDevice();

#ifdef _DEBUG
	enableDebugReport();
#endif

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

	createRenderPass();
	createFramebuffer();
	prepareCommandBuffers();
	prepareSemaphores();

	// VulkanAppBase���p�������N���X���Ƃ̏���������
	prepare();
}

void VulkanAppBase::terminate()
{
	// TODO:����͂Ȃ��K�v�H
	vkDeviceWaitIdle(m_device);

	// VulkanAppBase���p�������N���X���Ƃ̉������
	cleanup();

	vkFreeCommandBuffers(m_device, m_commandPool, uint32_t(m_commands.size()), m_commands.data());
	m_commands.clear();

	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	for (VkFramebuffer& v : m_framebuffers)
	{
		vkDestroyFramebuffer(m_device, v, nullptr);
	}
	m_framebuffers.clear();

	vkFreeMemory(m_device, m_depthBufferMemory, nullptr);
	vkDestroyImage(m_device, m_depthBuffer, nullptr);
	vkDestroyImageView(m_device, m_depthBufferView, nullptr);

	for (VkImageView& v : m_swapchainViews)
	{
		vkDestroyImageView(m_device, v, nullptr);
	}

	m_swapchainImages.clear(); // TODO:����VkImage�͉������Ȃ��H�X���b�v�`�F�C���������Ă邩��X���b�v�`�F�C���ƈꏏ�ɉ�������H
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

	for (VkFence& v : m_fences)
	{
		vkDestroyFence(m_device, v, nullptr);
	}
	m_fences.clear();
	vkDestroySemaphore(m_device, m_presentCompletedSem, nullptr);
	vkDestroySemaphore(m_device, m_renderCompletedSem, nullptr);

	vkDestroyCommandPool(m_device, m_commandPool, nullptr);

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_device, nullptr);

#ifdef _DEBUG
	disableDebugReport();
#endif

	vkDestroyInstance(m_instance, nullptr);
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

void VulkanAppBase::enableDebugReport()
{
	GetInstanceProcAddr(vkCreateDebugReportCallbackEXT);
	GetInstanceProcAddr(vkDebugReportMessageEXT);
	GetInstanceProcAddr(vkDestroyDebugReportCallbackEXT);

	VkDebugReportFlagsEXT flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	VkDebugReportCallbackCreateInfoEXT drcCI{};
	drcCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	drcCI.flags = flags;
	drcCI.pfnCallback = &DebugReportCallback;

	VkResult result = m_vkCreateDebugReportCallbackEXT(m_instance, &drcCI, nullptr, &m_debugReport);
	checkResult(result);
}

void VulkanAppBase::disableDebugReport()
{
	if (m_vkDestroyDebugReportCallbackEXT != nullptr)
	{
		m_vkDestroyDebugReportCallbackEXT(m_instance, m_debugReport, nullptr);
	}
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

void VulkanAppBase::createRenderPass()
{
	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	std::array<VkAttachmentDescription, 2> attachments;
	VkAttachmentDescription& colorTarget = attachments[0];
	VkAttachmentDescription& depthTarget = attachments[1];

	colorTarget = VkAttachmentDescription{};
	colorTarget.format = m_surfaceFormat.format;
	colorTarget.samples = VK_SAMPLE_COUNT_1_BIT;
	colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorTarget.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorTarget.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorTarget.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorTarget.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	depthTarget = VkAttachmentDescription{};
	depthTarget.format = VK_FORMAT_D32_SFLOAT;
	depthTarget.samples = VK_SAMPLE_COUNT_1_BIT;
	depthTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthTarget.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthTarget.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthTarget.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthTarget.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference{}, depthReference{};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // TODO:�Ȃ����̒l�ɁH

	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorReference;
	subpassDesc.pDepthStencilAttachment = &depthReference;

	ci.attachmentCount = uint32_t(attachments.size());
	ci.pAttachments = attachments.data();
	ci.subpassCount = 1;
	ci.pSubpasses = &subpassDesc;

	VkResult result = vkCreateRenderPass(m_device, &ci, nullptr, &m_renderPass);
	checkResult(result);
}

void VulkanAppBase::createFramebuffer()
{
	VkFramebufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	ci.renderPass = m_renderPass;
	ci.width = m_swapchainExtent.width;
	ci.height = m_swapchainExtent.height;
	ci.layers = 1;
	m_framebuffers.clear();

	// 2�����t���[���o�b�t�@�̓J���[���X���b�v�`�F�C���̃r���[�̂��ꂼ���
	// �f�v�X�X�e���V���͋��ʂ�1�����g��
	for (const VkImageView& v : m_swapchainViews)
	{
		std::array<VkImageView, 2> attachments;
		ci.attachmentCount = uint32_t(attachments.size());
		ci.pAttachments = attachments.data();
		attachments[0] = v;
		attachments[1] = m_depthBufferView;

		VkFramebuffer framebuffer = nullptr;
		VkResult result = vkCreateFramebuffer(m_device, &ci, nullptr, &framebuffer);
		checkResult(result);
		m_framebuffers.push_back(framebuffer);
	}
}

void VulkanAppBase::prepareCommandBuffers()
{
	VkCommandBufferAllocateInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.commandPool = m_commandPool;
	ai.commandBufferCount = uint32_t(m_swapchainViews.size());
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	m_commands.resize(ai.commandBufferCount);
	VkResult result = vkAllocateCommandBuffers(m_device, &ai, m_commands.data());
	checkResult(result);

	// �R�}���h�o�b�t�@�Ɠ����̃t�F���X��p�ӂ���
	m_fences.resize(ai.commandBufferCount);
	VkFenceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (VkFence& v : m_fences)
	{
		result = vkCreateFence(m_device, &ci, nullptr, &v);
		checkResult(result);
	}
}

void VulkanAppBase::prepareSemaphores()
{
	VkSemaphoreCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkResult result = vkCreateSemaphore(m_device, &ci, nullptr, &m_renderCompletedSem);
	checkResult(result);
	result = vkCreateSemaphore(m_device, &ci, nullptr, &m_presentCompletedSem);
	checkResult(result);
}

void VulkanAppBase::render()
{
	// �`���̎擾
	uint32_t nextImageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_presentCompletedSem, VK_NULL_HANDLE, &nextImageIndex);
	checkResult(result);

	const VkFence& commandFence = m_fences[nextImageIndex];
	result = vkWaitForFences(m_device, 1, &commandFence, VK_TRUE, UINT64_MAX);
	checkResult(result);

	// �N���A�l���_�u���o�b�t�@�p��2�p��
	std::array<VkClearValue, 2> clearValue = {
		{
			{0.5f, 0.25f, 0.25f, 0.0f}, // color
			{1.0f, 0}, // depth stencil
		}
	};

	// �R�}���h�o�b�t�@�ւ̃R�}���h�i�[�J�n�A���̒��̈�̃R�}���h�Ƃ��ă����_�[�p�X�J�n
	// �N���A���邾���̃����_�[�p�X����1�R�}���h�Ƃ��ăR�}���h�o�b�t�@�Ɋi�[������ȊO�̃R�}���h�͉����i�[���Ȃ�
	VkRenderPassBeginInfo renderPassBI{};
	renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBI.renderPass = m_renderPass;
	renderPassBI.framebuffer = m_framebuffers[nextImageIndex];
	renderPassBI.renderArea.offset = VkOffset2D{ 0, 0 };
	renderPassBI.renderArea.extent = m_swapchainExtent;
	renderPassBI.pClearValues = clearValue.data();
	renderPassBI.clearValueCount = uint32_t(clearValue.size());

	VkCommandBufferBeginInfo commandBI{};
	commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VkCommandBuffer& command = m_commands[nextImageIndex];
	result = vkBeginCommandBuffer(command, &commandBI);
	checkResult(result);
	vkCmdBeginRenderPass(command, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

	// VulkanAppBase���p�������N���X���Ƃ̃R�}���h�i�[����
	m_imageIndex = nextImageIndex;
	makeCommand(command);

	// �����_�[�p�X�I���A�R�}���h�o�b�t�@�ւ̃R�}���h�i�[�I��
	vkCmdEndRenderPass(command);
	result = vkEndCommandBuffer(command);
	checkResult(result);

	// �R�}���h�o�b�t�@���s
	VkSubmitInfo submitInfo{};
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command;
	submitInfo.pWaitDstStageMask = &waitStageMask;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_presentCompletedSem;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_renderCompletedSem;
	vkResetFences(m_device, 1, &commandFence);
	vkQueueSubmit(m_deviceQueue, 1, &submitInfo, commandFence);

	// Present(�t���b�v)����
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &nextImageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_renderCompletedSem;
	result = vkQueuePresentKHR(m_deviceQueue, &presentInfo);
	checkResult(result);
}


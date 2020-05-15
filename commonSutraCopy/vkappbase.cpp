#include "vkappbase.h"
#include <vector>

VulkanAppBase::VulkanAppBase()
{
}

void VulkanAppBase::intialize(GLFWwindow* window, const char* appName)
{
	initializeInstance(appName);
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

	// �g�����̎擾
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

void VulkanAppBase::prepare()
{
}

void VulkanAppBase::render()
{
}


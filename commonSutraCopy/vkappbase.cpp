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

}

void VulkanAppBase::prepare()
{
}

void VulkanAppBase::render()
{
}


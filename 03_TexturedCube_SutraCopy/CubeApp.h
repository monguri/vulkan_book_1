#pragma once

#include "../commonSutraCopy/vkappbase.h"
#include "glm/glm.hpp"

class CubeApp : public VulkanAppBase
{
protected:
	virtual void prepare() override;
	virtual void cleanup() override;
	virtual void makeCommand(VkCommandBuffer command) override;

private:
	struct BufferObject
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
	};

	BufferObject createBuffer(uint32_t size, VkBufferUsageFlags usage);
	VkPipelineShaderStageCreateInfo loadShaderModule(const char* fileName, VkShaderStageFlagBits stage);

	BufferObject m_vertexBuffer;
	BufferObject m_indexBuffer;

	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	uint32_t m_indexCount = 0;
};


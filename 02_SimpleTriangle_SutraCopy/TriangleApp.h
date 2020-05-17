#pragma once

#include "../commonSutraCopy/vkappbase.h"
#include "glm/glm.hpp"

class TriangleApp : public VulkanAppBase
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

	BufferObject m_vertexBuffer;
	BufferObject m_indexBuffer;
};


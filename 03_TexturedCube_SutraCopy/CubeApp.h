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
	struct CubeVertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;
	};

	struct ShaderParameters
	{
		glm::mat4 mtxWorld;
		glm::mat4 mtxView;
		glm::mat4 mtxProj;
	};

	struct BufferObject
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
	};

	void makeCubeGeometry();
	void prepareUniformBuffers();
	void prepareDescriptorSetLayout();
	void prepareDescriptorPool();
	void prepareDescriptorSet();

	BufferObject createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	VkPipelineShaderStageCreateInfo loadShaderModule(const char* fileName, VkShaderStageFlagBits stage);

	BufferObject m_vertexBuffer;
	BufferObject m_indexBuffer;
	std::vector<BufferObject> m_uniformBuffers;

	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	uint32_t m_indexCount = 0;

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
};


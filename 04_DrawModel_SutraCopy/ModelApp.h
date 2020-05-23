#pragma once

#include "../commonSutraCopy/vkappbase.h"
#include "glm/glm.hpp"
#include "GLTFSDK/GLTF.h"

namespace Microsoft
{
	namespace glTF
	{
		class Document;
		class GLTFResourceReader;
	}
};

class ModelApp : public VulkanAppBase
{
protected:
	virtual void prepare() override;
	virtual void cleanup() override;
	virtual void makeCommand(VkCommandBuffer command) override;

private:
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;
	};

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

	struct TextureObject
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
	};

	struct ModelMesh
	{
		BufferObject vertexBuffer;
		BufferObject indexBuffer;
		uint32_t vertexCount;
		uint32_t indexCount;
	};

	struct Model
	{
		std::vector<ModelMesh> meshes;
	};


	void makeModelGeometry(const Microsoft::glTF::Document& doc, std::shared_ptr<Microsoft::glTF::GLTFResourceReader> reader);
	void makeCubeGeometry();

	void prepareUniformBuffers();
	void prepareDescriptorSetLayout();
	void prepareDescriptorPool();
	void prepareDescriptorSet();

	BufferObject createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, const void* initialData = nullptr);
	VkPipelineShaderStageCreateInfo loadShaderModule(const char* fileName, VkShaderStageFlagBits stage);

	VkSampler createSampler();
	TextureObject createTexture(const char* fileName);
	void setImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	BufferObject m_vertexBuffer;
	BufferObject m_indexBuffer;
	std::vector<BufferObject> m_uniformBuffers;
	TextureObject m_texture;
	VkSampler m_sampler = VK_NULL_HANDLE;

	Model m_model;

	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	uint32_t m_indexCount = 0;

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_descriptorSet;
};


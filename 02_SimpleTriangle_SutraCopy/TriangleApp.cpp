#include "TriangleApp.h"
#include <array>

using namespace glm;

void TriangleApp::prepare()
{
	// 頂点バッファ、インデックスバッファ作成
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
	};

	const vec3 red(1.0f, 0.0f, 0.0f);
	const vec3 green(0.0f, 1.0f, 0.0f);
	const vec3 blue(0.0f, 0.0f, 1.0f);

	Vertex vertices[] = {
		{vec3(-1.0f, 0.0f, 0.0f), red}, // 左下
		{vec3(1.0f, 0.0f, 0.0f), green}, // 右下
		{vec3(0.0f, 1.0f, 0.0f), blue}, // 真ん中上
	};

	uint32_t indices[] = {0, 1, 2};

	m_vertexBuffer = createBuffer(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	m_indexBuffer = createBuffer(sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	// 頂点データ書き込み
	{
		void* p = nullptr;
		VkResult result = vkMapMemory(m_device, m_vertexBuffer.memory, 0, VK_WHOLE_SIZE, 0, &p);
		checkResult(result);
		memcpy(p, vertices, sizeof(vertices));
		vkUnmapMemory(m_device, m_vertexBuffer.memory);
	}

	// インデックスデータ書き込み
	{
		void* p = nullptr;
		VkResult result = vkMapMemory(m_device, m_indexBuffer.memory, 0, VK_WHOLE_SIZE, 0, &p);
		checkResult(result);
		memcpy(p, indices, sizeof(indices));
		vkUnmapMemory(m_device, m_indexBuffer.memory);
	}

	// 頂点の入力の設定
	VkVertexInputBindingDescription inputBinding{
		0, // binding
		sizeof(Vertex), // stride
		VK_VERTEX_INPUT_RATE_VERTEX // inputRate
	};

	std::array<VkVertexInputAttributeDescription, 2> inputAttribs{
		{
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
		}
	};

	VkPipelineVertexInputStateCreateInfo vertexInputCI{};
	vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCI.vertexBindingDescriptionCount = 1;
	vertexInputCI.pVertexBindingDescriptions = &inputBinding;
	vertexInputCI.vertexAttributeDescriptionCount = 1;
	vertexInputCI.pVertexAttributeDescriptions = inputAttribs.data();

	// ブレンディングの設定
	const int32_t colorWriteAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.blendEnable = VK_TRUE;
	blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.colorWriteMask = colorWriteAll;
	VkPipelineColorBlendStateCreateInfo cbCI{};
	cbCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cbCI.attachmentCount = 1;
	cbCI.pAttachments = &blendAttachment;
}

void TriangleApp::cleanup()
{
}

void TriangleApp::makeCommand(VkCommandBuffer command)
{
}

TriangleApp::BufferObject TriangleApp::createBuffer(uint32_t size, VkBufferUsageFlags usage)
{
	BufferObject obj;

	VkBufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	ci.usage = usage;
	ci.size = size;
	VkResult result = vkCreateBuffer(m_device, &ci, nullptr, &obj.buffer);
	checkResult(result);

	VkMemoryRequirements reqs;
	vkGetBufferMemoryRequirements(m_device, obj.buffer, &reqs);
	VkMemoryAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.allocationSize = reqs.size;
	VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	info.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, flags);
	result = vkAllocateMemory(m_device, &info, nullptr, &obj.memory);
	checkResult(result);

	result = vkBindBufferMemory(m_device, obj.buffer, obj.memory, 0);
	checkResult(result);

	return obj;
}

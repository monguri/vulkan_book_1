#include "CubeApp.h"
#include <array>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

void CubeApp::prepare()
{
	makeCubeGeometry();
	prepareUniformBuffers();
	prepareDescriptorSetLayout();
	prepareDescriptorPool();
	prepareDescriptorSet();

	// 頂点の入力の設定
	VkVertexInputBindingDescription inputBinding{
		0, // binding
		sizeof(CubeVertex), // stride
		VK_VERTEX_INPUT_RATE_VERTEX // inputRate
	};

	std::array<VkVertexInputAttributeDescription, 3> inputAttribs{
		{
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, pos)},
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, color)},
			{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, uv)},
		}
	};

	VkPipelineVertexInputStateCreateInfo vertexInputCI{};
	vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCI.vertexBindingDescriptionCount = 1;
	vertexInputCI.pVertexBindingDescriptions = &inputBinding;
	vertexInputCI.vertexAttributeDescriptionCount = uint32_t(inputAttribs.size());
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

	// ビューポートの設定
	VkViewport viewport;
	{
		viewport.x = 0.0f;
		viewport.y = float(m_swapchainExtent.height);
		viewport.width = float(m_swapchainExtent.width);
		viewport.height = -1.0f * float(m_swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.minDepth = 1.0f;
	}

	VkRect2D scissor = {
		{0, 0}, // offset
		m_swapchainExtent
	};

	VkPipelineViewportStateCreateInfo viewportCI{};
	viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCI.viewportCount = 1;
	viewportCI.pViewports = &viewport;
	viewportCI.scissorCount = 1;
	viewportCI.pScissors = &scissor;

	// プリミティブトポロジーの設定
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
	inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// ラスタライザーステート設定
	VkPipelineRasterizationStateCreateInfo rasterizerCI{};
	rasterizerCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCI.cullMode = VK_CULL_MODE_NONE;
	rasterizerCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCI.lineWidth = 1.0f;

	// マルチサンプル設定
	VkPipelineMultisampleStateCreateInfo multisampleCI{};
	multisampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	
	// デプスステンシルステート設定
	VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
	depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCI.depthTestEnable = VK_TRUE;
	depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilCI.depthWriteEnable = VK_TRUE;
	depthStencilCI.stencilTestEnable = VK_FALSE;

	// シェーダバイナリのロード
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages
	{
		loadShaderModule("shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
		loadShaderModule("shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
	};

	// パイプラインレイアウト
	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutCI, nullptr, &m_pipelineLayout);
	checkResult(result);

	// パイプライン構築
	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	ci.stageCount = uint32_t(shaderStages.size());
	ci.pStages = shaderStages.data();
	ci.pInputAssemblyState = &inputAssemblyCI;
	ci.pVertexInputState = &vertexInputCI;
	ci.pRasterizationState = &rasterizerCI;
	ci.pDepthStencilState = &depthStencilCI;
	ci.pMultisampleState = &multisampleCI;
	ci.pViewportState = &viewportCI;
	ci.pColorBlendState = &cbCI;
	ci.renderPass = m_renderPass;
	ci.layout = m_pipelineLayout;
	result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &ci, nullptr, &m_pipeline);
	checkResult(result);

	// シェーダバイナリはもう使わないので解放
	for (const VkPipelineShaderStageCreateInfo& v : shaderStages)
	{
		vkDestroyShaderModule(m_device, v.module, nullptr);
	}
}

void CubeApp::cleanup()
{
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_device, m_pipeline, nullptr);
	vkFreeMemory(m_device, m_vertexBuffer.memory, nullptr);
	vkFreeMemory(m_device, m_indexBuffer.memory, nullptr);
	vkDestroyBuffer(m_device, m_vertexBuffer.buffer, nullptr);
	vkDestroyBuffer(m_device, m_indexBuffer.buffer, nullptr);
}

void CubeApp::makeCommand(VkCommandBuffer command)
{
	// ユニフォームバッファの中身を更新する
	ShaderParameters shaderParam{};
	shaderParam.mtxWorld = rotate(identity<glm::mat4>(), radians(45.0f), vec3(0, 1, 0));
	shaderParam.mtxView = lookAtRH(vec3(0.0f, 3.0f, 5.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	shaderParam.mtxProj = perspective(radians(60.0f), 640.0f / 480.0f, 0.01f, 100.0f);

	{
		const VkDeviceMemory& memory = m_uniformBuffers[m_imageIndex].memory;
		void* p = nullptr;
		VkResult result = vkMapMemory(m_device, memory, 0, VK_WHOLE_SIZE, 0, &p);
		checkResult(result);
		memcpy(p, &shaderParam, sizeof(shaderParam));
		vkUnmapMemory(m_device, memory);
	}

	vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command, 0, 1, &m_vertexBuffer.buffer, &offset);
	vkCmdBindIndexBuffer(command, m_indexBuffer.buffer, offset, VK_INDEX_TYPE_UINT32);

	// ディクリプタセットをセット
	VkDescriptorSet descriptorSets[] = {
		m_descriptorSet[m_imageIndex]
	};
	vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, descriptorSets, 0, nullptr);

	vkCmdDrawIndexed(command, m_indexCount, 1, 0, 0, 0);
}

void CubeApp::makeCubeGeometry()
{
	const float k = 1.0f;
	const vec3 red(1.0f, 0.0f, 0.0f);
	const vec3 green(0.0f, 1.0f, 0.0f);
	const vec3 blue(0.0f, 0.0f, 1.0f);
	const vec3 white(1.0f);
	const vec3 black(0.0f);
	const vec3 yellow(1.0f, 1.0f, 0.0f);
	const vec3 magenta(1.0f, 0.0f, 1.0f);
	const vec3 cyan(0.0f, 1.0f, 1.0f);

	// この本だとUVは左下が(0,0)にして、ビューポートの設定で逆になるように扱う
	const vec2 lb(0.0f, 0.0f);
	const vec2 lt(0.0f, 1.0f);
	const vec2 rb(1.0f, 0.0f);
	const vec2 rt(1.0f, 1.0f);

	CubeVertex vertices[] = {
		// 正面
		{vec3(-k, k, k), yellow, lb},
		{vec3(-k, -k, k), red, lt},
		{vec3(k, k, k), white, rb},
		{vec3(k, -k, k), magenta, rt},
		// 右
		{vec3(k, k, k), white, lb},
		{vec3(k, -k, k), magenta, lt},
		{vec3(k, k, -k), cyan, rb},
		{vec3(k, -k, -k), blue, rt},
		// 左
		{vec3(-k, k, -k), green, lb},
		{vec3(-k, -k, -k), black, lt},
		{vec3(-k, k, k), yellow, rb},
		{vec3(-k, -k, k), red, rt},
		// 裏
		{vec3(k, k, -k), cyan, lb},
		{vec3(k, -k, -k), blue, lt},
		{vec3(-k, k, -k), green, rb},
		{vec3(-k, -k, -k), black, rt},
		// 上
		{vec3(-k, k, -k), green, lb},
		{vec3(-k, k, k), yellow, lt},
		{vec3(k, k, -k), cyan, rb},
		{vec3(k, k, k), white, rt},
		// 底
		{vec3(-k, -k, k), red, lb},
		{vec3(-k, -k, -k), black, lt},
		{vec3(k, -k, k), magenta, rb},
		{vec3(k, -k, -k), blue, rt},
	};

	uint32_t indices[] = {
		0, 2, 1, 1, 2, 3, // 正面
		4, 6, 5, 5, 6, 7, // 右
		8, 10, 9, 9, 10, 11, // 左
		12, 14, 13, 13, 14, 15, // 裏
		16, 18, 17, 17, 18, 19, // 上
		20, 22, 21, 21, 22, 23, // 底
	};

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

	m_indexCount = _countof(indices);
}

void CubeApp::prepareUniformBuffers()
{
	// TODO:なぜ定数バッファがダブルバッファにひとつずつ必要なのか？
	m_uniformBuffers.resize(m_swapchainViews.size());
	for (BufferObject& v : m_uniformBuffers)
	{
		VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		v = createBuffer(sizeof(ShaderParameters), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags);
	}
}

void CubeApp::prepareDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	VkDescriptorSetLayoutBinding bindingUBO{};
	bindingUBO.binding = 0;
	bindingUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindingUBO.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindingUBO.descriptorCount = 1;
	bindings.push_back(bindingUBO);

	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.bindingCount = uint32_t(bindings.size());
	ci.pBindings = bindings.data();
	VkResult result = vkCreateDescriptorSetLayout(m_device, &ci, nullptr, &m_descriptorSetLayout);
	checkResult(result);
}

void CubeApp::prepareDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 1> descPoolSize;
	descPoolSize[0].descriptorCount = uint32_t(m_uniformBuffers.size());
	descPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	VkDescriptorPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = uint32_t(m_uniformBuffers.size());
	ci.poolSizeCount = uint32_t(descPoolSize.size());
	ci.pPoolSizes = descPoolSize.data();
	VkResult result = vkCreateDescriptorPool(m_device, &ci, nullptr, &m_descriptorPool);
	checkResult(result);
}

void CubeApp::prepareDescriptorSet()
{
	// TODO:なぜ定数バッファがダブルバッファにひとつずつ必要なのか？
	std::vector<VkDescriptorSetLayout> layouts;
	for (size_t i = 0; i < m_uniformBuffers.size(); ++i)
	{
		layouts.push_back(m_descriptorSetLayout);
	}

	VkDescriptorSetAllocateInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	ai.descriptorPool = m_descriptorPool;
	ai.descriptorSetCount = uint32_t(m_uniformBuffers.size());
	ai.pSetLayouts = layouts.data();

	m_descriptorSet.resize(m_uniformBuffers.size());
	VkResult result = vkAllocateDescriptorSets(m_device, &ai, m_descriptorSet.data());
	checkResult(result);

	// ディスクリプタセットへ書き込み
	for (size_t i = 0; i < m_uniformBuffers.size(); ++i)
	{
		VkDescriptorBufferInfo descUBO{};
		descUBO.buffer = m_uniformBuffers[i].buffer;
		descUBO.offset = 0;
		descUBO.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet ubo{};
		ubo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ubo.dstBinding = 0;
		ubo.descriptorCount = 1;
		ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo.pBufferInfo = &descUBO;
		ubo.dstSet = m_descriptorSet[i];

		std::vector<VkWriteDescriptorSet> writeSets = {
			ubo
		};
		vkUpdateDescriptorSets(m_device, uint32_t(writeSets.size()), writeSets.data(), 0, nullptr);
	}
}

CubeApp::BufferObject CubeApp::createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags)
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
	info.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, flags);
	result = vkAllocateMemory(m_device, &info, nullptr, &obj.memory);
	checkResult(result);

	result = vkBindBufferMemory(m_device, obj.buffer, obj.memory, 0);
	checkResult(result);

	return obj;
}

VkPipelineShaderStageCreateInfo CubeApp::loadShaderModule(const char* fileName, VkShaderStageFlagBits stage)
{
	std::ifstream infile(fileName, std::ios::binary);
	if (!infile)
	{
		OutputDebugStringA("file not found.\n");
		DebugBreak();
	}

	std::vector<char> filedata;
	filedata.resize(uint32_t(infile.seekg(0, std::ifstream::end).tellg()));
	infile.seekg(0, std::ifstream::beg).read(filedata.data(), filedata.size());

	VkShaderModuleCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ci.pCode = reinterpret_cast<uint32_t*>(filedata.data());
	ci.codeSize = filedata.size();

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(m_device, &ci, nullptr, &shaderModule);
	checkResult(result);

	VkPipelineShaderStageCreateInfo shaderStageCI{};
	shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCI.stage = stage;
	shaderStageCI.module = shaderModule;
	shaderStageCI.pName = "main";

	return shaderStageCI;
}


#include "ModelApp.h"
#include <array>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../commonSutraCopy/stb_image.h"

#include "streamreader.h"

using namespace glm;

void ModelApp::prepare()
{
	// モデルデータの読み込み
	std::experimental::filesystem::path modelFilePath =  std::experimental::filesystem::path("alicia-solid.vrm");
	if (modelFilePath.is_relative())
	{
		std::experimental::filesystem::path current = std::experimental::filesystem::current_path();
		current /= modelFilePath;
		current.swap(modelFilePath);
	}

	std::unique_ptr<StreamReader> reader = std::make_unique<StreamReader>(modelFilePath.parent_path());
	std::shared_ptr<std::istream> glbStream = reader->GetInputStream(modelFilePath.filename().u8string());
	std::shared_ptr<Microsoft::glTF::GLBResourceReader> glbResourceReader = std::make_shared<Microsoft::glTF::GLBResourceReader>(std::move(reader), std::move(glbStream));
	const Microsoft::glTF::Document& document = Microsoft::glTF::Deserialize(glbResourceReader->GetJson());


	makeModelGeometry(document, glbResourceReader);
	makeModelMaterial(document, glbResourceReader);


	makeCubeGeometry();
	prepareUniformBuffers();
	prepareDescriptorSetLayout();
	prepareDescriptorPool();

	m_texture = createTexture("texture.tga");
	m_sampler = createSampler();

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

	// ビューポートの設定
	VkViewport viewport;
	{
		viewport.x = 0.0f;
		viewport.y = float(m_swapchainExtent.height);
		viewport.width = float(m_swapchainExtent.width);
		viewport.height = -1.0f * float(m_swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
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
	
	// パイプラインレイアウト
	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &m_descriptorSetLayout;
	VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutCI, nullptr, &m_pipelineLayout);
	checkResult(result);

	// 不透明用: パイプラインの構築
	{
		// ブレンディングの設定
		const int32_t colorWriteAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		VkPipelineColorBlendAttachmentState blendAttachment{};
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.colorWriteMask = colorWriteAll;
		VkPipelineColorBlendStateCreateInfo cbCI{};
		cbCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cbCI.attachmentCount = 1;
		cbCI.pAttachments = &blendAttachment;

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
		result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &ci, nullptr, &m_pipelineOpaque);
		checkResult(result);

		// シェーダバイナリはもう使わないので解放
		for (const VkPipelineShaderStageCreateInfo& v : shaderStages)
		{
			vkDestroyShaderModule(m_device, v.module, nullptr);
		}
	}

	// 半透明用: パイプラインの構築
	{
		// ブレンディングの設定
		const int32_t colorWriteAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		VkPipelineColorBlendAttachmentState blendAttachment{};
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.colorWriteMask = colorWriteAll;
		VkPipelineColorBlendStateCreateInfo cbCI{};
		cbCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cbCI.attachmentCount = 1;
		cbCI.pAttachments = &blendAttachment;

		// デプスステンシルステート設定
		VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
		depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCI.depthTestEnable = VK_TRUE;
		depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilCI.depthWriteEnable = VK_FALSE;
		depthStencilCI.stencilTestEnable = VK_FALSE;

		// シェーダバイナリのロード
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages
		{
			loadShaderModule("shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShaderModule("shaderAlpha.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
		};

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
		result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &ci, nullptr, &m_pipelineAlpha);
		checkResult(result);

		// シェーダバイナリはもう使わないので解放
		for (const VkPipelineShaderStageCreateInfo& v : shaderStages)
		{
			vkDestroyShaderModule(m_device, v.module, nullptr);
		}
	}
}

void ModelApp::cleanup()
{
	for (BufferObject& v : m_uniformBuffers)
	{
		vkDestroyBuffer(m_device, v.buffer, nullptr);
		vkFreeMemory(m_device, v.memory, nullptr);
	}

	vkDestroySampler(m_device, m_sampler, nullptr);
	vkDestroyImage(m_device, m_texture.image, nullptr);
	vkDestroyImageView(m_device, m_texture.view, nullptr);
	vkFreeMemory(m_device, m_texture.memory, nullptr);

	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_device, m_pipelineOpaque, nullptr);
	vkDestroyPipeline(m_device, m_pipelineAlpha, nullptr);

	vkFreeMemory(m_device, m_vertexBuffer.memory, nullptr);
	vkFreeMemory(m_device, m_indexBuffer.memory, nullptr);
	vkDestroyBuffer(m_device, m_vertexBuffer.buffer, nullptr);
	vkDestroyBuffer(m_device, m_indexBuffer.buffer, nullptr);

	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
}

void ModelApp::makeCommand(VkCommandBuffer command)
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

	vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineOpaque);

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

void ModelApp::makeModelGeometry(const Microsoft::glTF::Document& doc, std::shared_ptr<Microsoft::glTF::GLTFResourceReader> reader)
{
	using namespace Microsoft::glTF;

	for (const Mesh& mesh : doc.meshes.Elements())
	{
		for (const MeshPrimitive& meshPrimitive : mesh.primitives)
		{
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			// 頂点位置情報アクセサの取得
			const std::string& idPos = meshPrimitive.GetAttributeAccessorId(ACCESSOR_POSITION);
			const Accessor& accPos = doc.accessors.Get(idPos);

			// 法線情報アクセサの取得
			const std::string& idNrm = meshPrimitive.GetAttributeAccessorId(ACCESSOR_NORMAL);
			const Accessor& accNrm = doc.accessors.Get(idNrm);

			// UV情報アクセサの取得
			const std::string& idUV = meshPrimitive.GetAttributeAccessorId(ACCESSOR_NORMAL);
			const Accessor& accUV = doc.accessors.Get(idUV);

			// インデックス情報アクセサの取得
			const std::string& idIndex = meshPrimitive.indicesAccessorId;
			const Accessor& accIndex = doc.accessors.Get(idIndex);

			// アクセサからデータ列を取得
			const std::vector<float> vertPos = reader->ReadBinaryData<float>(doc, accPos);
			const std::vector<float> vertNrm = reader->ReadBinaryData<float>(doc, accNrm);
			const std::vector<float> vertUV = reader->ReadBinaryData<float>(doc, accUV);

			size_t vertexCount = accPos.count;
			for (uint32_t i = 0; i < vertexCount; ++i)
			{
				// 頂点データの構築
				int vid0 = 3 * i, vid1 = 3 * i + 1, vid2 = 3 * i + 2;
				int tid0 = 2 * i, tid1 = 2 * i + 1;
				vertices.emplace_back(
					Vertex{
						vec3(vertPos[vid0], vertPos[vid1], vertPos[vid2]),
						vec3(vertNrm[vid0], vertNrm[vid1], vertNrm[vid2]),
						vec2(vertUV[tid0], vertUV[tid1]),
					}
				);
			}

			// インデックスデータはアクセサから取得した配列をそのまま使えばよい
			indices = reader->ReadBinaryData<uint32_t>(doc, accIndex);

			ModelMesh modelMesh;
			uint32_t vbSize = uint32_t(sizeof(Vertex) * vertices.size());
			uint32_t ibSize = uint32_t(sizeof(uint32_t) * indices.size());

			modelMesh.vertexBuffer = createBuffer(vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertices.data());
			modelMesh.indexBuffer = createBuffer(ibSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indices.data());
			modelMesh.vertexCount = uint32_t(vertices.size());
			modelMesh.indexCount = uint32_t(indices.size());
			modelMesh.materialIndex = int(doc.materials.GetIndex(meshPrimitive.materialId));

			m_model.meshes.push_back(modelMesh);
		}
	}
}

void ModelApp::makeModelMaterial(const Microsoft::glTF::Document& doc, std::shared_ptr<Microsoft::glTF::GLTFResourceReader> reader)
{
	using namespace Microsoft::glTF;

	for (const Microsoft::glTF::Material& m : doc.materials.Elements())
	{
		std::string textureId = m.metallicRoughness.baseColorTexture.textureId;
		if (textureId.empty())
		{
			// TODO:なぜmetallicRoughnessになければnormalTextureなのか。。
			textureId = m.normalTexture.textureId;
		}

		const Texture& texture = doc.textures.Get(textureId);
		const Image& image = doc.images.Get(texture.imageId);
		const BufferView& imageBufferView = doc.bufferViews.Get(image.bufferViewId);
		const std::vector<char>& imageData = reader->ReadBinaryData<char>(doc, imageBufferView);

		Material material{};
		material.alphaMode = m.alphaMode;
		material.texture = createTextureFromMemory(imageData);
		m_model.materials.push_back(material);
	}
}

void ModelApp::makeCubeGeometry()
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

void ModelApp::prepareUniformBuffers()
{
	// TODO:なぜ定数バッファがダブルバッファにひとつずつ必要なのか？
	m_uniformBuffers.resize(m_swapchainViews.size());
	for (BufferObject& v : m_uniformBuffers)
	{
		VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		v = createBuffer(sizeof(ShaderParameters), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags);
	}
}

void ModelApp::prepareDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	VkDescriptorSetLayoutBinding bindingUBO{}, bindingTex{};
	bindingUBO.binding = 0;
	bindingUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindingUBO.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindingUBO.descriptorCount = 1;
	bindings.push_back(bindingUBO);

	bindingTex.binding = 1;
	bindingTex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingTex.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindingTex.descriptorCount = 1;
	bindings.push_back(bindingTex);

	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.bindingCount = uint32_t(bindings.size());
	ci.pBindings = bindings.data();
	VkResult result = vkCreateDescriptorSetLayout(m_device, &ci, nullptr, &m_descriptorSetLayout);
	checkResult(result);
}

void ModelApp::prepareDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> descPoolSize;
	descPoolSize[0].descriptorCount = uint32_t(m_uniformBuffers.size());
	descPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descPoolSize[1].descriptorCount = uint32_t(m_uniformBuffers.size());
	descPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = uint32_t(m_uniformBuffers.size());
	ci.poolSizeCount = uint32_t(descPoolSize.size());
	ci.pPoolSizes = descPoolSize.data();
	VkResult result = vkCreateDescriptorPool(m_device, &ci, nullptr, &m_descriptorPool);
	checkResult(result);
}

void ModelApp::prepareDescriptorSet()
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

		VkDescriptorImageInfo descImage{};
		descImage.imageView = m_texture.view;
		descImage.sampler = m_sampler;
		descImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet ubo{};
		ubo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ubo.dstBinding = 0;
		ubo.descriptorCount = 1;
		ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo.pBufferInfo = &descUBO;
		ubo.dstSet = m_descriptorSet[i];

		VkWriteDescriptorSet tex{};
		tex.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		tex.dstBinding = 0;
		tex.descriptorCount = 1;
		tex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		tex.pImageInfo = &descImage;
		tex.dstSet = m_descriptorSet[i];

		std::vector<VkWriteDescriptorSet> writeSets = {
			ubo, tex
		};
		vkUpdateDescriptorSets(m_device, uint32_t(writeSets.size()), writeSets.data(), 0, nullptr);
	}
}

ModelApp::BufferObject ModelApp::createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, const void* initialData)
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

	if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 && initialData != nullptr)
	{
		void* p = nullptr;
		VkResult result = vkMapMemory(m_device, obj.memory, 0, VK_WHOLE_SIZE, 0, &p);
		checkResult(result);
		memcpy(p, initialData, size);
		vkUnmapMemory(m_device, obj.memory);
	}

	return obj;
}

VkPipelineShaderStageCreateInfo ModelApp::loadShaderModule(const char* fileName, VkShaderStageFlagBits stage)
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

VkSampler ModelApp::createSampler()
{
	VkSampler sampler;
	VkSamplerCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	ci.minFilter = VK_FILTER_LINEAR;
	ci.magFilter = VK_FILTER_LINEAR;
	ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ci.maxAnisotropy = 1.0f;
	ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VkResult result = vkCreateSampler(m_device, &ci, nullptr, &sampler);
	checkResult(result);
	return sampler;
}

ModelApp::TextureObject ModelApp::createTextureFromMemory(const std::vector<char>& imageData)
{
	BufferObject stagingBuffer;
	TextureObject texture{};

	// 画像データをstbライブラリでロード
	int width, height, channels;
	stbi_uc* pImage = stbi_load_from_memory((stbi_uc*)(imageData.data()), int(imageData.size()), &width, &height, &channels, 0);
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	{
		// テクスチャのVkImageを生成
		VkImageCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ci.extent = { uint32_t(width), uint32_t(height), 1 };
		ci.format = format;
		ci.imageType = VK_IMAGE_TYPE_2D;
		ci.mipLevels = 1;
		ci.samples = VK_SAMPLE_COUNT_1_BIT;
		ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ci.arrayLayers = 1;
		VkResult result = vkCreateImage(m_device, &ci, nullptr, &texture.image);
		checkResult(result);

		// メモリ量の算出
		VkMemoryRequirements reqs;
		vkGetImageMemoryRequirements(m_device, texture.image, &reqs);
		VkMemoryAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize = reqs.size;
		// メモリタイプの判定
		info.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		// メモリの確保
		result = vkAllocateMemory(m_device, &info, nullptr, &texture.memory);
		checkResult(result);
		// メモリのバインド
		result = vkBindImageMemory(m_device, texture.image, texture.memory, 0);
		checkResult(result);
	}

	{
		uint32_t imageSize = width * height * sizeof(uint32_t);
		// ステージングバッファを用意
		stagingBuffer = createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, pImage);
	}

	VkCommandBuffer command;
	{
		VkCommandBufferAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		ai.commandBufferCount = 1;
		ai.commandPool = m_commandPool;
		ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VkResult result = vkAllocateCommandBuffers(m_device, &ai, &command);
		checkResult(result);
	}

	VkCommandBufferBeginInfo commandBI{};
	commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VkResult result = vkBeginCommandBuffer(command, &commandBI);
	checkResult(result);

	setImageMemoryBarrier(command, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBufferImageCopy copyRegion{};
	copyRegion.imageExtent = { uint32_t(width), uint32_t(height), 1 };
	copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
	vkCmdCopyBufferToImage(command, stagingBuffer.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	setImageMemoryBarrier(command, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	result = vkEndCommandBuffer(command);
	checkResult(result);

	// コマンドバッファ実行
	VkSubmitInfo submitInfo{};
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command;
	vkQueueSubmit(m_deviceQueue, 1, &submitInfo, VK_NULL_HANDLE);
	{
		// テクスチャ参照用のビューを作成
		VkImageViewCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ci.image = texture.image;
		ci.format = format;
		ci.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
		};
		ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		VkResult result = vkCreateImageView(m_device, &ci, nullptr, &texture.view);
		checkResult(result);
	}

	vkDeviceWaitIdle(m_device);
	vkFreeCommandBuffers(m_device, m_commandPool, uint32_t(m_commands.size()), m_commands.data());

	// ステージングバッファ解放
	vkFreeMemory(m_device, stagingBuffer.memory, nullptr);
	vkDestroyBuffer(m_device, stagingBuffer.buffer, nullptr);

	return texture;
}

ModelApp::TextureObject ModelApp::createTexture(const char* fileName)
{
	BufferObject stagingBuffer;
	TextureObject texture{};

	// 画像データをstbライブラリでロード
	int width, height, channels;
	stbi_uc* pImage = stbi_load(fileName, &width, &height, &channels, 0);
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	{
		// テクスチャのVkImageを生成
		VkImageCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ci.extent = { uint32_t(width), uint32_t(height), 1 };
		ci.format = format;
		ci.imageType = VK_IMAGE_TYPE_2D;
		ci.mipLevels = 1;
		ci.samples = VK_SAMPLE_COUNT_1_BIT;
		ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ci.arrayLayers = 1;
		VkResult result = vkCreateImage(m_device, &ci, nullptr, &texture.image);
		checkResult(result);

		// メモリ量の算出
		VkMemoryRequirements reqs;
		vkGetImageMemoryRequirements(m_device, texture.image, &reqs);
		VkMemoryAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize = reqs.size;
		// メモリタイプの判定
		info.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		// メモリの確保
		result = vkAllocateMemory(m_device, &info, nullptr, &texture.memory);
		checkResult(result);
		// メモリのバインド
		result = vkBindImageMemory(m_device, texture.image, texture.memory, 0);
		checkResult(result);
	}

	{
		uint32_t imageSize = width * height * sizeof(uint32_t);
		// ステージングバッファを用意
		stagingBuffer = createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		// ステージングバッファに画像データを書き込み
		void* p = nullptr;
		VkResult result = vkMapMemory(m_device, stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &p);
		checkResult(result);
		memcpy(p, pImage, imageSize);
		vkUnmapMemory(m_device, stagingBuffer.memory);
	}

	VkCommandBuffer command;
	{
		VkCommandBufferAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		ai.commandBufferCount = 1;
		ai.commandPool = m_commandPool;
		ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VkResult result = vkAllocateCommandBuffers(m_device, &ai, &command);
		checkResult(result);
	}

	VkCommandBufferBeginInfo commandBI{};
	commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VkResult result = vkBeginCommandBuffer(command, &commandBI);
	checkResult(result);

	setImageMemoryBarrier(command, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBufferImageCopy copyRegion{};
	copyRegion.imageExtent = { uint32_t(width), uint32_t(height), 1 };
	copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
	vkCmdCopyBufferToImage(command, stagingBuffer.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	setImageMemoryBarrier(command, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	result = vkEndCommandBuffer(command);
	checkResult(result);

	// コマンドバッファ実行
	VkSubmitInfo submitInfo{};
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command;
	vkQueueSubmit(m_deviceQueue, 1, &submitInfo, VK_NULL_HANDLE);
	{
		// テクスチャ参照用のビューを作成
		VkImageViewCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ci.image = texture.image;
		ci.format = format;
		ci.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
		};
		ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		VkResult result = vkCreateImageView(m_device, &ci, nullptr, &texture.view);
		checkResult(result);
	}

	vkDeviceWaitIdle(m_device);
	vkFreeCommandBuffers(m_device, m_commandPool, uint32_t(m_commands.size()), m_commands.data());

	// ステージングバッファ解放
	vkFreeMemory(m_device, stagingBuffer.memory, nullptr);
	vkDestroyBuffer(m_device, stagingBuffer.buffer, nullptr);

	stbi_image_free(pImage);
	return texture;
}

void ModelApp::setImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier imb{};
	imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imb.oldLayout = oldLayout;
	imb.newLayout = newLayout;
	imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imb.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	imb.image = image;

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	switch (oldLayout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			imb.srcAccessMask = 0;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
	}

	switch (newLayout)
	{
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			imb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			imb.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
	}

	vkCmdPipelineBarrier(
		command,
		srcStage,
		dstStage,
		0,
		0, // memoryBarrierCount
		nullptr,
		0, // bufferMemoryBarrierCount
		nullptr,
		1, // imageMemoryBarrierCount
		&imb
	);
}


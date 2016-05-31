/*
* Vulkan Demo Scene 
*
* Don't take this a an example, it's more of a personal playground
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* Note : Different license than the other examples!
*
* This code is licensed under the Mozilla Public License Version 2.0 (http://opensource.org/licenses/MPL-2.0)
*/

#include "vulkanexamplebase.h"

static std::vector<std::string> names { "logos", "background", "models", "skybox" };

class VulkanExample : public VulkanExampleBase
{
public:

	struct DemoMeshes
	{
		vk::PipelineVertexInputStateCreateInfo inputState;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
		vk::Pipeline pipeline;
		VulkanMeshLoader* logos;
		VulkanMeshLoader* background;
		VulkanMeshLoader* models;
		VulkanMeshLoader* skybox;
	} demoMeshes;
	std::vector<VulkanMeshLoader*> meshes;

	struct {
		vkTools::UniformData meshVS;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 normal;
		glm::mat4 view;
		glm::vec4 lightPos;
	} uboVS;

	struct
	{
		vkTools::VulkanTexture skybox;
	} textures;

	struct {
		vk::Pipeline logos;
		vk::Pipeline models;
		vk::Pipeline skybox;
	} pipelines;

	vk::PipelineLayout pipelineLayout;
	vk::DescriptorSet descriptorSet;
	vk::DescriptorSetLayout descriptorSetLayout;

	glm::vec4 lightPos = glm::vec4(1.0f, 2.0f, 0.0f, 0.0f);

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		width = 1280;
		height = 720;
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Vulkan Demo Scene - � 2016 by Sascha Willems";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		device.destroyPipeline(pipelines.logos);
		device.destroyPipeline(pipelines.models);
		device.destroyPipeline(pipelines.skybox);

		device.destroyPipelineLayout(pipelineLayout);
		device.destroyDescriptorSetLayout(descriptorSetLayout);

		vkTools::destroyUniformData(device, &uniformData.meshVS);

		for (auto& mesh : meshes)
		{
			device.destroyBuffer(mesh->vertexBuffer.buf);
			device.freeMemory(mesh->vertexBuffer.mem);

			device.destroyBuffer(mesh->indexBuffer.buf);
			device.freeMemory(mesh->indexBuffer.mem);
		}

		textureLoader->destroyTexture(textures.skybox);
	
		delete(demoMeshes.logos);
		delete(demoMeshes.background);
		delete(demoMeshes.models);
		delete(demoMeshes.skybox);
	}

	void loadTextures()
	{
		textureLoader->loadCubemap(
			getAssetPath() + "textures/cubemap_vulkan.ktx", 
			vk::Format::eR8G8B8A8Unorm, 
			&textures.skybox);
	}

	void buildCommandBuffers()
	{
		vk::CommandBufferBeginInfo cmdBufInfo;

		vk::ClearValue clearValues[2];
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		vk::RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			drawCmdBuffers[i].begin(cmdBufInfo);
			

			drawCmdBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

			vk::Viewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			drawCmdBuffers[i].setViewport(0, viewport);

			vk::Rect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
			drawCmdBuffers[i].setScissor(0, scissor);

			drawCmdBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);

			vk::DeviceSize offsets = 0;
			for (auto& mesh : meshes)
			{
				drawCmdBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, mesh->pipeline);
				drawCmdBuffers[i].bindVertexBuffers(VERTEX_BUFFER_BIND_ID, mesh->vertexBuffer.buf, offsets);
				drawCmdBuffers[i].bindIndexBuffer(mesh->indexBuffer.buf, 0, vk::IndexType::eUint32);
				drawCmdBuffers[i].drawIndexed(mesh->indexBuffer.count, 1, 0, 0, 0);
			}

			drawCmdBuffers[i].endRenderPass();

			drawCmdBuffers[i].end();
			
		}
	}

	void draw()
	{
		// Get next image in the swap chain (back/front buffer)
		prepareFrame();
		// Command buffer to be sumitted to the queue
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to queue
		queue.submit(submitInfo, VK_NULL_HANDLE);
		

		submitFrame();
		
	}

	void prepareVertices()
	{
		struct Vertex {
			float pos[3];
			float normal[3];
			float uv[2];
			float color[3];
		};

		// Load meshes for demos scene
		demoMeshes.logos = new VulkanMeshLoader();
		demoMeshes.background = new VulkanMeshLoader();
		demoMeshes.models = new VulkanMeshLoader();
		demoMeshes.skybox = new VulkanMeshLoader();

#if defined(__ANDROID__)
		demoMeshes.logos->assetManager = androidApp->activity->assetManager;
		demoMeshes.background->assetManager = androidApp->activity->assetManager;
		demoMeshes.models->assetManager = androidApp->activity->assetManager;
		demoMeshes.skybox->assetManager = androidApp->activity->assetManager;
#endif

		demoMeshes.logos->LoadMesh(getAssetPath() + "models/vulkanscenelogos.dae");
		demoMeshes.background->LoadMesh(getAssetPath() + "models/vulkanscenebackground.dae");
		demoMeshes.models->LoadMesh(getAssetPath() + "models/vulkanscenemodels.dae");
		demoMeshes.skybox->LoadMesh(getAssetPath() + "models/cube.obj");

		std::vector<VulkanMeshLoader*> meshList;
		meshList.push_back(demoMeshes.skybox); // skybox first because of depth writes
		meshList.push_back(demoMeshes.logos);
		meshList.push_back(demoMeshes.background);
		meshList.push_back(demoMeshes.models);

		// todo : Use mesh function for loading
		float scale = 1.0f;
		for (auto& mesh : meshList)
		{
			// Generate vertex buffer (pos, normal, uv, color)
			std::vector<Vertex> vertexBuffer;
			for (int m = 0; m < mesh->m_Entries.size(); m++)
			{
				for (int i = 0; i < mesh->m_Entries[m].Vertices.size(); i++) {
					glm::vec3 pos = mesh->m_Entries[m].Vertices[i].m_pos * scale;
					glm::vec3 normal = mesh->m_Entries[m].Vertices[i].m_normal;
					glm::vec2 uv = mesh->m_Entries[m].Vertices[i].m_tex;
					glm::vec3 col = mesh->m_Entries[m].Vertices[i].m_color;
					Vertex vert = {
						{ pos.x, pos.y, pos.z },
						{ normal.x, -normal.y, normal.z },
						{ uv.s, uv.t },
						{ col.r, col.g, col.b }
					};

					// Offset Vulkan meshes
					// todo : center before export
					if (mesh != demoMeshes.skybox)
					{
						vert.pos[1] += 1.15f;
					}

					vertexBuffer.push_back(vert);
				}
			}
		createBuffer(vk::BufferUsageFlagBits::eVertexBuffer,
			vertexBuffer.size() * sizeof(Vertex),
			vertexBuffer.data(),
			mesh->vertexBuffer.buf,
			mesh->vertexBuffer.mem);

			uint32_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

			std::vector<uint32_t> indexBuffer;
			for (int m = 0; m < mesh->m_Entries.size(); m++)
			{
				int indexBase = indexBuffer.size();
				for (int i = 0; i < mesh->m_Entries[m].Indices.size(); i++) {
					indexBuffer.push_back(mesh->m_Entries[m].Indices[i] + indexBase);
				}
			}
		createBuffer(vk::BufferUsageFlagBits::eVertexBuffer,
			indexBuffer.size() * sizeof(uint32_t),
			indexBuffer.data(),
			mesh->indexBuffer.buf,
			mesh->indexBuffer.mem);
			mesh->indexBuffer.count = indexBuffer.size();

			meshes.push_back(mesh);
		}

		// Binding description
		demoMeshes.bindingDescriptions.resize(1);
		demoMeshes.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex), vk::VertexInputRate::eVertex);

		// Attribute descriptions
		// Location 0 : Position
		demoMeshes.attributeDescriptions.resize(4);
		demoMeshes.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, vk::Format::eR32G32B32Sfloat, 0);
		// Location 1 : Normal
		demoMeshes.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3);
		// Location 2 : Texture coordinates
		demoMeshes.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, vk::Format::eR32G32Sfloat, sizeof(float) * 6);
		// Location 3 : Color
		demoMeshes.attributeDescriptions[3] =
			vkTools::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, vk::Format::eR32G32B32Sfloat, sizeof(float) * 8);

		demoMeshes.inputState.vertexBindingDescriptionCount = demoMeshes.bindingDescriptions.size();
		demoMeshes.inputState.pVertexBindingDescriptions = demoMeshes.bindingDescriptions.data();
		demoMeshes.inputState.vertexAttributeDescriptionCount = demoMeshes.attributeDescriptions.size();
		demoMeshes.inputState.pVertexAttributeDescriptions = demoMeshes.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo and one image sampler
		std::vector<vk::DescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),
			vkTools::initializers::descriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
		};

		vk::DescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 2);

		descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
	}

	void setupDescriptorSetLayout()
	{
		std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				vk::DescriptorType::eUniformBuffer,
				vk::ShaderStageFlagBits::eVertex,
				0),
			// Binding 1 : Fragment shader color map image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				vk::DescriptorType::eCombinedImageSampler,
				vk::ShaderStageFlagBits::eFragment,
				1)
		};

		vk::DescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), setLayoutBindings.size());

		descriptorSetLayout = device.createDescriptorSetLayout(descriptorLayout);
		

		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);

		pipelineLayout = device.createPipelineLayout(pPipelineLayoutCreateInfo);
		
	}

	void setupDescriptorSet()
	{
		vk::DescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

		descriptorSet = device.allocateDescriptorSets(allocInfo)[0];

		// Cube map image descriptor
		vk::DescriptorImageInfo texDescriptorCubeMap =
			vkTools::initializers::descriptorImageInfo(textures.skybox.sampler, textures.skybox.view, vk::ImageLayout::eGeneral);

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				vk::DescriptorType::eUniformBuffer,
				0,
				&uniformData.meshVS.descriptor),
			// Binding 1 : Fragment shader image sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				vk::DescriptorType::eCombinedImageSampler,
				1,
				&texDescriptorCubeMap)
		};

		device.updateDescriptorSets(writeDescriptorSets, nullptr);
	}

	void preparePipelines()
	{
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;

		vk::PipelineRasterizationStateCreateInfo rasterizationState =
			vkTools::initializers::pipelineRasterizationStateCreateInfo(
				vk::PolygonMode::eFill,
				vk::CullModeFlagBits::eBack,
				vk::FrontFace::eClockwise);

		vk::PipelineColorBlendAttachmentState blendAttachmentState;
		blendAttachmentState.colorWriteMask = vkTools::initializers::fullColorWriteMask();

		vk::PipelineColorBlendStateCreateInfo colorBlendState;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;

		vk::PipelineViewportStateCreateInfo viewportState;
		viewportState.scissorCount = 1;
		viewportState.viewportCount = 1;

		vk::PipelineMultisampleStateCreateInfo multisampleState;

		std::vector<vk::DynamicState> dynamicStateEnables = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};
		vk::PipelineDynamicStateCreateInfo dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());

		// Pipeline for the meshes (armadillo, bunny, etc.)
		// Load shaders
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0] = loadShader(getAssetPath() + "shaders/vulkanscene/mesh.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/vulkanscene/mesh.frag.spv", vk::ShaderStageFlagBits::eFragment);

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(pipelineLayout, renderPass);

		pipelineCreateInfo.pVertexInputState = &demoMeshes.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		pipelines.models = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];
		

		// Pipeline for the logos
		shaderStages[0] = loadShader(getAssetPath() + "shaders/vulkanscene/logo.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/vulkanscene/logo.frag.spv", vk::ShaderStageFlagBits::eFragment);
		pipelines.logos = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];
		

		// Pipeline for the sky sphere (todo)
		rasterizationState.cullMode = vk::CullModeFlagBits::eFront; // Inverted culling
		depthStencilState.depthWriteEnable = VK_FALSE; // No depth writes
		shaderStages[0] = loadShader(getAssetPath() + "shaders/vulkanscene/skybox.vert.spv", vk::ShaderStageFlagBits::eVertex);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/vulkanscene/skybox.frag.spv", vk::ShaderStageFlagBits::eFragment);
		pipelines.skybox = device.createGraphicsPipelines(pipelineCache, pipelineCreateInfo, nullptr)[0];
		

		// Assign pipelines
		demoMeshes.logos->pipeline = pipelines.logos;
		demoMeshes.models->pipeline = pipelines.models;
		demoMeshes.background->pipeline = pipelines.models;
		demoMeshes.skybox->pipeline = pipelines.skybox;
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		createBuffer(vk::BufferUsageFlagBits::eUniformBuffer,
			sizeof(uboVS),
			&uboVS,
			uniformData.meshVS.buffer,
			uniformData.meshVS.memory,
			uniformData.meshVS.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);

		uboVS.view = glm::lookAt(
			glm::vec3(0, 0, -zoom),
			glm::vec3(0, 0, 0),
			glm::vec3(0, 1, 0)
			);

		uboVS.model = glm::mat4();
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboVS.normal = glm::inverseTranspose(uboVS.view * uboVS.model);

		uboVS.lightPos = lightPos;

		void *pData = device.mapMemory(uniformData.meshVS.memory, 0, sizeof(uboVS), vk::MemoryMapFlags());
		memcpy(pData, &uboVS, sizeof(uboVS));
		device.unmapMemory(uniformData.meshVS.memory);
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadTextures();
		prepareVertices();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		vkDeviceWaitIdle(device);
		draw();
		vkDeviceWaitIdle(device);
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

};

RUN_EXAMPLE(VulkanExample)
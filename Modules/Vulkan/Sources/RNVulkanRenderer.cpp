//
//  RNVulkanRenderer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanRenderer.h"
#include "RNVulkanWindow.h"
#include "RNVulkanInternals.h"
#include "RNVulkanGPUBuffer.h"
#include "RNVulkanShader.h"
#include "RNVulkanShaderLibrary.h"

namespace RN
{
	RNDefineMeta(VulkanRenderer, Renderer)

	VulkanRenderer::VulkanRenderer(VulkanRendererDescriptor *descriptor, VulkanDevice *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_textureFormatLookup(new Dictionary()),
		_currentFrame(0),
		_defaultShaders(new Dictionary()),
		_mipMapTextures(new Set()),
		_submittedCommandBuffers(new Array())
	{
		vk::GetDeviceQueue(device->GetDevice(), device->GetWorkQueue(), 0, &_workQueue);

		_internals->stateCoordinator.SetRenderer(this);
		_internals->renderPass.drawableHead = nullptr;

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->GetWorkQueue();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		RNVulkanValidate(vk::CreateCommandPool(device->GetDevice(), &cmdPoolInfo, nullptr, &_commandPool));

#define TextureFormat(name, vulkan) \
			case Texture::Format::name: { \
				VkFormatProperties properties; \
				vk::GetPhysicalDeviceFormatProperties(device->GetPhysicalDevice(), vulkan, &properties); \
				 \
				VkFormatFeatureFlags required = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT; \
				 \
				if((properties.optimalTilingFeatures & required) == required) \
					_textureFormatLookup->SetObjectForKey(Number::WithUint32(vulkan), RNCSTR(#name)); \
			} break

		bool isDone = false;

		for(size_t i = 0; isDone == false; i ++)
		{
			switch(static_cast<Texture::Format>(i))
			{
				TextureFormat(RGBA8888, VK_FORMAT_R8G8B8A8_UNORM);
				TextureFormat(RGB10A2, VK_FORMAT_A2R10G10B10_UNORM_PACK32);

				TextureFormat(R8, VK_FORMAT_R8_UNORM);
				TextureFormat(RG88, VK_FORMAT_R8G8_UNORM);
				TextureFormat(RGB888, VK_FORMAT_R8G8B8_UNORM);

				TextureFormat(R16F, VK_FORMAT_R16_SFLOAT);
				TextureFormat(RG16F, VK_FORMAT_R16G16_SFLOAT);
				TextureFormat(RGB16F, VK_FORMAT_R16G16B16_SFLOAT);
				TextureFormat(RGBA16F, VK_FORMAT_R16G16B16A16_SFLOAT);

				TextureFormat(R32F, VK_FORMAT_R32_SFLOAT);
				TextureFormat(RG32F, VK_FORMAT_R32G32_SFLOAT);
				TextureFormat(RGB32F, VK_FORMAT_R32G32B32_SFLOAT);
				TextureFormat(RGBA32F, VK_FORMAT_R32G32B32A32_SFLOAT);

				TextureFormat(Depth24I, VK_FORMAT_X8_D24_UNORM_PACK32);
				TextureFormat(Depth32F, VK_FORMAT_D32_SFLOAT);
				TextureFormat(Stencil8, VK_FORMAT_S8_UINT);
				TextureFormat(Depth24Stencil8, VK_FORMAT_D24_UNORM_S8_UINT);
				TextureFormat(Depth32FStencil8, VK_FORMAT_D32_SFLOAT_S8_UINT);

				case Texture::Format::Invalid:
					isDone = true;
					break;
			}
		}
#undef TextureFormat
	}

	VulkanRenderer::~VulkanRenderer()
	{
		_mipMapTextures->Release();
		_textureFormatLookup->Release();
		_defaultShaders->Release();
	}

	void VulkanRenderer::CreateVulkanCommandBuffers(size_t count, std::vector<VkCommandBuffer> &buffers)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = _commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(count);

		buffers.resize(count);
		RNVulkanValidate(vk::AllocateCommandBuffers(GetVulkanDevice()->GetDevice(), &commandBufferAllocateInfo, buffers.data()));
	}

	VkCommandBuffer VulkanRenderer::CreateVulkanCommandBuffer()
	{
		std::vector<VkCommandBuffer> buffers;
		CreateVulkanCommandBuffers(1, buffers);

		return buffers[0];
	}

	VulkanCommandBuffer *VulkanRenderer::GetCommandBuffer()
	{
		VulkanCommandBuffer *commandBuffer = new VulkanCommandBuffer(GetVulkanDevice()->GetDevice(), _commandPool);
		_lock.Lock();
		commandBuffer->_commandBuffer = CreateVulkanCommandBuffer();
		_lock.Unlock();

		return commandBuffer->Autorelease();
	}

	VulkanCommandBufferWithCallback *VulkanRenderer::GetCommandBufferWithCallback()
	{
		VulkanCommandBufferWithCallback *commandBuffer = new VulkanCommandBufferWithCallback(GetVulkanDevice()->GetDevice(), _commandPool);
		_lock.Lock();
		commandBuffer->_commandBuffer = CreateVulkanCommandBuffer();
		_lock.Unlock();

		return commandBuffer->Autorelease();
	}

	void VulkanRenderer::SubmitCommandBuffer(VulkanCommandBuffer *commandBuffer)
	{
		_lock.Lock();
		_submittedCommandBuffers->AddObject(commandBuffer);
		_lock.Unlock();
	}

	void VulkanRenderer::ProcessCommandBuffers()
	{
		std::vector<VkCommandBuffer> buffers;
		_lock.Lock();
		if(_submittedCommandBuffers->GetCount() == 0)
		{
			_lock.Unlock();
			return;
		}

		buffers.reserve(_submittedCommandBuffers->GetCount());
		_submittedCommandBuffers->Enumerate<VulkanCommandBuffer>([&](VulkanCommandBuffer *buffer, int i, bool &stop){
			buffers.push_back(buffer->_commandBuffer);
		});
		_lock.Unlock();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = buffers.size();
		submitInfo.pCommandBuffers = buffers.data();

		RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, VK_NULL_HANDLE));
		RNVulkanValidate(vk::QueueWaitIdle(_workQueue));

		_submittedCommandBuffers->RemoveAllObjects();
	}

	Window *VulkanRenderer::CreateAWindow(const Vector2 &size, Screen *screen)
	{
		RN_ASSERT(!_mainWindow, "Vulkan renderer only supports one window");

		_mainWindow = new VulkanWindow(size, screen, this);
		return _mainWindow;
	}
	Window *VulkanRenderer::GetMainWindow()
	{
		return _mainWindow;
	}

	void VulkanRenderer::RenderIntoWindow(Window *twindow, Function &&function)
	{
		//CreateMipMaps(); // Needs the global command buffer
		ProcessCommandBuffers();

		VulkanWindow *window = static_cast<VulkanWindow *>(twindow);

		window->AcquireBackBuffer();
		function();
		window->PresentBackBuffer();

		vk::DeviceWaitIdle(GetVulkanDevice()->GetDevice());
		_currentFrame ++;
	}
	void VulkanRenderer::RenderIntoCamera(Camera *camera, Function &&function)
	{
		_internals->renderPass.drawableHead = nullptr;

		_internals->renderPass.viewMatrix = camera->GetViewMatrix();
		_internals->renderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		_internals->renderPass.projectionMatrix = camera->GetProjectionMatrix();
		_internals->renderPass.projectionMatrix.m[5] *= -1.0f;
		_internals->renderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		_internals->renderPass.projectionViewMatrix = _internals->renderPass.projectionMatrix * _internals->renderPass.viewMatrix;

		// Submit drawables
		function();

		VulkanFramebuffer *framebuffer = static_cast<VulkanFramebuffer *>(camera->GetFramebuffer());
		Vector2 size = _mainWindow->GetSize();

		//Happens to be 0 when the window is closed.
		if(size.x < 0.01f || size.y < 0.01f)
		{
			size.x = 1.0f;
			size.y = 1.0f;
		}

		if(!framebuffer)
			framebuffer = _mainWindow->GetFramebuffer();

		VulkanBackBuffer *backbuffer = _mainWindow->GetActiveBackbuffer();
		uint32_t framebufferIndex = backbuffer->GetImageIndex();

		VulkanCommandBuffer *bufferObject = GetCommandBuffer();
		bufferObject->Begin();
		VkCommandBuffer commandBuffer = bufferObject->GetCommandBuffer();

		VkImageMemoryBarrier postPresentBarrier = {};
		postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		postPresentBarrier.pNext = NULL;
		postPresentBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		postPresentBarrier.image = static_cast<VulkanTexture *>(framebuffer->GetColorTexture(framebufferIndex))->GetImage();

		vk::CmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &postPresentBarrier);


		VkClearValue clearValues[2];
		clearValues[0].color = {camera->GetClearColor().r, camera->GetClearColor().g, camera->GetClearColor().b,
								camera->GetClearColor().a};
		clearValues[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = NULL;
		renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
		renderPassBeginInfo.framebuffer = framebuffer->GetFramebuffer(framebufferIndex);
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(size.x);
		renderPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(size.y);
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		vk::CmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.width = size.x;
		viewport.height = size.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::CmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = static_cast<uint32_t>(size.x);
		scissor.extent.height = static_cast<uint32_t>(size.y);
		scissor.offset.x = 0;
		scissor.offset.y = 0;

		vk::CmdSetScissor(commandBuffer, 0, 1, &scissor);

		VulkanDrawable *drawable = _internals->renderPass.drawableHead;
		while(drawable)
		{
			RenderDrawable(commandBuffer, drawable);
			drawable = drawable->_next;
		}

		vk::CmdEndRenderPass(commandBuffer);

		// Add a present memory barrier to the end of the command buffer
		// This will transform the frame buffer color attachment to a
		// new layout for presenting it to the windowing system integration
		VkImageMemoryBarrier prePresentBarrier = {};
		prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		prePresentBarrier.pNext = NULL;
		prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		prePresentBarrier.image = static_cast<VulkanTexture *>(framebuffer->GetColorTexture(framebufferIndex))->GetImage();

		vk::CmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &prePresentBarrier);
		bufferObject->End();

		VkSemaphore presentSemaphore = backbuffer->GetPresentSemaphore();
		VkSemaphore renderSemaphore = backbuffer->GetRenderSemaphore();
		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentSemaphore;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderSemaphore;
		submitInfo.pWaitDstStageMask = &pipelineStageFlags;

		RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, VK_NULL_HANDLE));
	}

	bool VulkanRenderer::SupportsTextureFormat(const String *format) const
	{
		return false;
	}
	bool VulkanRenderer::SupportsDrawMode(DrawMode mode) const
	{
		return false;
	}

	size_t VulkanRenderer::GetAlignmentForType(PrimitiveType type) const
	{
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
				return 4;

			case PrimitiveType::Vector2:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Matrix:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;
		}
	}
	size_t VulkanRenderer::GetSizeForType(PrimitiveType type) const
	{
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
				return 4;

			case PrimitiveType::Vector2:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;

			case PrimitiveType::Matrix:
				return 64;
		}
	}
	const String *VulkanRenderer::GetTextureFormatName(const Texture::Format format) const
	{
#define TextureFormatX(name) \
		case Texture::Format::name: \
			return RNCSTR(#name) \

		switch(format)
		{
			TextureFormatX(RGBA8888);
			TextureFormatX(RGB10A2);

			TextureFormatX(R8);
			TextureFormatX(RG88);
			TextureFormatX(RGB888);

			TextureFormatX(R16F);
			TextureFormatX(RG16F);
			TextureFormatX(RGB16F);
			TextureFormatX(RGBA16F);

			TextureFormatX(R32F);
			TextureFormatX(RG32F);
			TextureFormatX(RGB32F);
			TextureFormatX(RGBA32F);

			TextureFormatX(Depth24I);
			TextureFormatX(Depth32F);
			TextureFormatX(Stencil8);
			TextureFormatX(Depth24Stencil8);
			TextureFormatX(Depth32FStencil8);

			case Texture::Format::Invalid:
				return nullptr;
		}

#undef TextureFormatX
	}

	void VulkanRenderer::CreateMipMapForTexture(VulkanTexture *texture)
	{
		_lock.Lock();
		_mipMapTextures->AddObject(texture);
		_lock.Unlock();
	}

	void VulkanRenderer::CreateMipMaps()
	{
		if(_mipMapTextures->GetCount() == 0)
			return;

		VulkanCommandBuffer *commandBuffer = GetCommandBuffer();
		commandBuffer->Begin();

		_mipMapTextures->Enumerate<VulkanTexture>([&](VulkanTexture *texture, bool &stop) {

			VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetImage(), 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetImage(), 1, texture->GetDescriptor().mipMaps-1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			for(uint16 i = 0; i < texture->GetDescriptor().mipMaps-1; i++)
			{
				VkImageBlit imageBlit = {};

				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.mipLevel = i;
				imageBlit.srcSubresource.baseArrayLayer = 0;
				imageBlit.srcSubresource.layerCount = 1;

				imageBlit.srcOffsets[0].x = 0;
				imageBlit.srcOffsets[0].y = 0;
				imageBlit.srcOffsets[0].z = 0;
				imageBlit.srcOffsets[1].x = texture->GetDescriptor().GetWidthForMipMapLevel(i);
				imageBlit.srcOffsets[1].y = texture->GetDescriptor().GetHeightForMipMapLevel(i);
				imageBlit.srcOffsets[1].z = 1;

				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.mipLevel = i+1;
				imageBlit.dstSubresource.baseArrayLayer = 0;
				imageBlit.dstSubresource.layerCount = 1;

				imageBlit.dstOffsets[0].x = 0;
				imageBlit.dstOffsets[0].y = 0;
				imageBlit.dstOffsets[0].z = 0;
				imageBlit.dstOffsets[1].x = texture->GetDescriptor().GetWidthForMipMapLevel(i+1);
				imageBlit.dstOffsets[1].y = texture->GetDescriptor().GetHeightForMipMapLevel(i+1);
				imageBlit.dstOffsets[1].z = 1;

				vk::CmdBlitImage(commandBuffer->GetCommandBuffer(), texture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, texture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, 1, &imageBlit, VK_FILTER_LINEAR);

				VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetImage(), i+1, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			}

			VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetImage(), 0, texture->GetDescriptor().mipMaps, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

		commandBuffer->End();
		SubmitCommandBuffer(commandBuffer);

		_mipMapTextures->RemoveAllObjects();
	}

	VkFormat VulkanRenderer::GetVulkanFormatForName(const String *name)
	{
		Number *value = _textureFormatLookup->GetObjectForKey<Number>(name);
		if(value)
			return static_cast<VkFormat>(value->GetUint32Value());

		throw InvalidTextureFormatException(RNSTR("Unsupported texture format '" << name << "'"));
	}

	GPUBuffer *VulkanRenderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		void *data = malloc(length);
		return (new VulkanGPUBuffer(this, data, length, usageOptions));
	}
	GPUBuffer *VulkanRenderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		void *data = malloc(length);
		memcpy(data, bytes, length);
		return (new VulkanGPUBuffer(this, data, length, usageOptions));
	}

	ShaderLibrary *VulkanRenderer::CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options)
	{
		VulkanShaderLibrary *library = new VulkanShaderLibrary(this, file, options);
		return library;
	}
	ShaderLibrary *VulkanRenderer::CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options)
	{
		RN_ASSERT(-1, "NOT IMPLEMENTED!");
		return nullptr;
	}

	ShaderProgram *VulkanRenderer::GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup)
	{
		Dictionary *defines = new Dictionary();

		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Normals))
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_NORMALS"));
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Tangents))
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_TANGENTS"));
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Color0))
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_COLOR"));
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::UVCoords0))
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_UV0"));

		if(lookup->discard)
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_DISCARD"));

		ShaderCompileOptions *options = new ShaderCompileOptions();
		options->SetDefines(defines);

		ShaderLibrary *library;

		{
			LockGuard<Lockable> lock(_lock);
			library = _defaultShaders->GetObjectForKey<ShaderLibrary>(options);

			if(!library)
			{
				library = CreateShaderLibraryWithFile(RNCSTR(":RayneVulkan:/Shaders.json"), options);
				_defaultShaders->SetObjectForKey(library, options);
			}
		}

		options->Release();
		defines->Release();

		Shader *vertex = library->GetShaderWithName(RNCSTR("gouraud_vertex"));
		Shader *fragment = library->GetShaderWithName(RNCSTR("gouraud_fragment"));

		ShaderProgram *program = new ShaderProgram(vertex, fragment);
		return program->Autorelease();
	}

	Texture *VulkanRenderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		VulkanTexture *texture = new VulkanTexture(descriptor, this);
		return texture;
	}

	Framebuffer *VulkanRenderer::CreateFramebuffer(const Vector2 &size, const Framebuffer::Descriptor &descriptor)
	{
		return new VulkanFramebuffer(size, descriptor,  _mainWindow->GetSwapChain(), this);
	}

	void VulkanRenderer::FillUniformBuffer(GPUBuffer *uniformBuffer, VulkanDrawable *drawable)
	{
		GPUBuffer *gpuBuffer = uniformBuffer;//uniformBuffer->Advance();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer());

		Matrix result = _internals->renderPass.projectionViewMatrix * drawable->modelMatrix;
		std::memcpy(buffer, result.m, sizeof(Matrix));

	/*	const VulkanUniformBuffer::Member *member;*/

		// Matrices
//		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ModelMatrix)))
		{
			std::memcpy(buffer + sizeof(Matrix), drawable->modelMatrix.m, sizeof(Matrix));
		}
/*		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ModelViewMatrix)))
		{
			Matrix result = _internals->renderPass.viewMatrix * drawable->modelMatrix;
			std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ModelViewProjectionMatrix)))
		{
			Matrix result = _internals->renderPass.projectionViewMatrix * drawable->modelMatrix;
			std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ViewMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), _internals->renderPass.viewMatrix.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ViewProjectionMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), _internals->renderPass.projectionViewMatrix.m, sizeof(Matrix));
		}

		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseModelMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), drawable->inverseModelMatrix.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseModelViewMatrix)))
		{
			Matrix result = _internals->renderPass.inverseViewMatrix * drawable->inverseModelMatrix;
			std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseModelViewProjectionMatrix)))
		{
			Matrix result = _internals->renderPass.inverseProjectionViewMatrix * drawable->inverseModelMatrix;
			std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseViewMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), _internals->renderPass.inverseViewMatrix.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseViewProjectionMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), _internals->renderPass.inverseProjectionViewMatrix.m, sizeof(Matrix));
		}*/

		// Color
		Material *material = drawable->material;

//		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::AmbientColor)))
		{
			std::memcpy(buffer + sizeof(Matrix)*2, &material->GetAmbientColor().r, sizeof(Color));
		}
//		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::DiffuseColor)))
		{
			std::memcpy(buffer + sizeof(Matrix)*2 + sizeof(Color), &material->GetDiffuseColor().r, sizeof(Color));
		}
/*		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::SpecularColor)))
		{
			std::memcpy(buffer + member->GetOffset(), &material->GetSpecularColor().r, sizeof(Color));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::EmissiveColor)))
		{
			std::memcpy(buffer + member->GetOffset(), &material->GetEmissiveColor().r, sizeof(Color));
		}

		// Misc
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::DiscardThreshold)))
		{
			float temp = material->GetDiscardThreshold();
			std::memcpy(buffer + member->GetOffset(), &temp, sizeof(float));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::TextureTileFactor)))
		{
			float temp = material->GetTextureTileFactor();
			std::memcpy(buffer + member->GetOffset(), &temp, sizeof(float));
		}*/

		gpuBuffer->Invalidate();
	}

	Drawable *VulkanRenderer::CreateDrawable()
	{
		VulkanDrawable *drawable = new VulkanDrawable();
		drawable->_pipelineState = nullptr;
		drawable->_next = nullptr;
		drawable->_prev = nullptr;

		return drawable;
	}

	void VulkanRenderer::SubmitDrawable(Drawable *tdrawable)
	{
		VulkanDrawable *drawable = static_cast<VulkanDrawable *>(tdrawable);

		if(drawable->dirty)
		{
			//TODO: Fix the camera situation...
			_lock.Lock();
			const VulkanPipelineState *pipelineState = _internals->stateCoordinator.GetRenderPipelineState(drawable->material, drawable->mesh, nullptr);
			VulkanUniformState *uniformState = _internals->stateCoordinator.GetUniformStateForPipelineState(pipelineState, drawable->material);
			_lock.Unlock();

			drawable->UpdateRenderingState(this, pipelineState, uniformState);
			drawable->dirty = false;
		}

		FillUniformBuffer(drawable->_uniformState->uniformBuffer, drawable);

		// Push into the queue
		drawable->_prev = nullptr;

		_lock.Lock();

		drawable->_next = _internals->renderPass.drawableHead;

		if(drawable->_next)
			drawable->_next->_prev = drawable;

		_internals->renderPass.drawableHead = drawable;
		_internals->renderPass.drawableCount ++;

		_lock.Unlock();
	}

	void VulkanRenderer::RenderDrawable(VkCommandBuffer commandBuffer, VulkanDrawable *drawable)
	{
		vk::CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawable->_pipelineState->pipelineLayout, 0, 1, &drawable->_uniformState->descriptorSet, 0, NULL);
		vk::CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawable->_pipelineState->state);

		VulkanGPUBuffer *buffer = static_cast<VulkanGPUBuffer *>(drawable->mesh->GetVertexBuffer());
		VulkanGPUBuffer *indices = static_cast<VulkanGPUBuffer *>(drawable->mesh->GetIndicesBuffer());

		VkDeviceSize offsets[1] = { 0 };
		// Bind mesh vertex buffer
		vk::CmdBindVertexBuffers(commandBuffer, 0, 1, &buffer->_buffer, offsets);
		// Bind mesh index buffer
		vk::CmdBindIndexBuffer(commandBuffer, indices->_buffer, 0, VK_INDEX_TYPE_UINT16);
		// Render mesh vertex buffer using it's indices
		vk::CmdDrawIndexed(commandBuffer, drawable->mesh->GetIndicesCount(), 1, 0, 0, 0);
	}
}

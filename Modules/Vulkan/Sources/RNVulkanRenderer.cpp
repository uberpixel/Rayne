//
//  RNVulkanRenderer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanRenderer.h"
#include "RNVulkanWindow.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanTexture.h"

namespace RN
{
	RNDefineMeta(VulkanRenderer, Renderer)

	VulkanRenderer::VulkanRenderer(VulkanRendererDescriptor *descriptor, VulkanDevice *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_textureFormatLookup(new Dictionary()),
		_currentFrame(0)
	{
		vk::GetDeviceQueue(device->GetDevice(), device->GetWorkQueue(), 0, &_workQueue);

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

		CreateCommandBuffer(_commandBuffer);

		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		RNVulkanValidate(vk::BeginCommandBuffer(_commandBuffer, &cmdBufInfo));
	}

	VkResult VulkanRenderer::CreateCommandBuffers(size_t count, std::vector<VkCommandBuffer> &buffers)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = _commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(count);

		buffers.resize(count);
		return vk::AllocateCommandBuffers(GetVulkanDevice()->GetDevice(), &commandBufferAllocateInfo, buffers.data());
	}

	VkResult VulkanRenderer::CreateCommandBuffer(VkCommandBuffer &buffer)
	{
		std::vector<VkCommandBuffer> buffers;
		VkResult result = CreateCommandBuffers(1, buffers);

		RNVulkanValidate(result);

		if(result == VK_SUCCESS)
			buffer = buffers.at(0);

		return result;
	}

	void VulkanRenderer::SubmitGlobalCommandBuffer()
	{
		RNVulkanValidate(vk::EndCommandBuffer(_commandBuffer));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_commandBuffer;

		RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, VK_NULL_HANDLE));
		RNVulkanValidate(vk::QueueWaitIdle(_workQueue));

		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		RNVulkanValidate(vk::BeginCommandBuffer(_commandBuffer, &cmdBufInfo));
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
		SubmitGlobalCommandBuffer();

		VulkanWindow *window = static_cast<VulkanWindow *>(twindow);

		window->AcquireBackBuffer();
		function();
		window->PresentBackBuffer();

		vk::DeviceWaitIdle(GetVulkanDevice()->GetDevice());
		_currentFrame ++;
	}
	void VulkanRenderer::RenderIntoCamera(Camera *camera, Function &&function)
	{
		VulkanFramebuffer *framebuffer = static_cast<VulkanFramebuffer *>(camera->GetFramebuffer());
		Vector2 size = _mainWindow->GetSize();

		if(!framebuffer)
			framebuffer = _mainWindow->GetFramebuffer();

		VkCommandBuffer commandBuffer = framebuffer->GetCommandBuffer();

		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.pNext = NULL;

		RNVulkanValidate(vk::BeginCommandBuffer(commandBuffer, &cmdBufInfo));


		/*VkClearValue clearValues[2];
		clearValues[0].color = { 0.0f, 0.5f, 0.9f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = NULL;
		renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
		renderPassBeginInfo.framebuffer = framebuffer->GetFramebuffer();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(size.x);
		renderPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(size.y);
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = clearValues;

		vk::CmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.height = size.x;
		viewport.width = size.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::CmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = static_cast<uint32_t>(size.x);
		scissor.extent.height = static_cast<uint32_t>(size.y);
		scissor.offset.x = 0;
		scissor.offset.y = 0;

		vk::CmdSetScissor(commandBuffer, 0, 1, &scissor);*/

		function();

		//vk::CmdEndRenderPass(commandBuffer);

		// Add a present memory barrier to the end of the command buffer
		// This will transform the frame buffer color attachment to a
		// new layout for presenting it to the windowing system integration
		VkImageMemoryBarrier prePresentBarrier = {};
		prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		prePresentBarrier.pNext = NULL;
		prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		prePresentBarrier.dstAccessMask = 0;
		prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		prePresentBarrier.image = static_cast<VulkanTexture *>(framebuffer->GetColorTexture())->GetImage();

		vk::CmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &prePresentBarrier);
		vk::EndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, VK_NULL_HANDLE));

		VkImageMemoryBarrier postPresentBarrier = {};
		postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		postPresentBarrier.pNext = NULL;
		postPresentBarrier.srcAccessMask = 0;
		postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		postPresentBarrier.image = static_cast<VulkanTexture *>(framebuffer->GetColorTexture())->GetImage();

		vk::CmdPipelineBarrier(GetGlobalCommandBuffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &postPresentBarrier);
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
		return 0;
	}
	size_t VulkanRenderer::GetSizeForType(PrimitiveType type) const
	{
		return 0;
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
	VkFormat VulkanRenderer::GetVulkanFormatForName(const String *name)
	{
		Number *value = _textureFormatLookup->GetObjectForKey<Number>(name);
		if(value)
			return static_cast<VkFormat>(value->GetUint32Value());

		throw InvalidTextureFormatException(RNSTR("Unsupported texture format '" << name << "'"));
	}

	GPUBuffer *VulkanRenderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions options)
	{
		return nullptr;
	}
	GPUBuffer *VulkanRenderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options)
	{
		return nullptr;
	}

	ShaderLibrary *VulkanRenderer::CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options)
	{
		return nullptr;
	}
	ShaderLibrary *VulkanRenderer::CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options)
	{
		return nullptr;
	}

	ShaderProgram *VulkanRenderer::GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup)
	{
		return nullptr;
	}

	Texture *VulkanRenderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		return nullptr;
	}

	Framebuffer *VulkanRenderer::CreateFramebuffer(const Vector2 &size, const Framebuffer::Descriptor &descriptor)
	{
		return new VulkanFramebuffer(size, descriptor,  _mainWindow->GetSwapChain(), this);
	}

	Drawable *VulkanRenderer::CreateDrawable()
	{
		return nullptr;
	}
	void VulkanRenderer::SubmitDrawable(Drawable *drawable)
	{}
}

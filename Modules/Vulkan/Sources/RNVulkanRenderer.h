//
//  RNVulkanRenderer.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANRENDERER_H_
#define __RAYNE_VULKANRENDERER_H_

#include "RNVulkan.h"
#include "RNVulkanDevice.h"
#include "RNVulkanRendererDescriptor.h"
#include "RNVulkanBackBuffer.h"

namespace RN
{
	class VulkanWindow;
	struct VulkanRendererInternals;
	struct VulkanDrawable;

	class VulkanRenderer : public Renderer
	{
	public:
		VKAPI VulkanRenderer(VulkanRendererDescriptor *descriptor, VulkanDevice *device);

		VKAPI Window *CreateAWindow(const Vector2 &size, Screen *screen) final;
		VKAPI Window *GetMainWindow() final;

		VKAPI void RenderIntoWindow(Window *window, Function &&function) final;
		VKAPI void RenderIntoCamera(Camera *camera, Function &&function) final;

		VKAPI bool SupportsTextureFormat(const String *format) const final;
		VKAPI bool SupportsDrawMode(DrawMode mode) const final;

		VKAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		VKAPI size_t GetSizeForType(PrimitiveType type) const final;
		VKAPI const String *GetTextureFormatName(const Texture::Format format) const final;
		VKAPI VkFormat GetVulkanFormatForName(const String *name);

		VKAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions) final;
		VKAPI GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options, GPUResource::AccessOptions accessOptions) final;

		VKAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options) final;
		VKAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options) final;

		VKAPI ShaderProgram *GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup) final;

		VKAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;

		VKAPI Framebuffer *CreateFramebuffer(const Vector2 &size, const Framebuffer::Descriptor &descriptor) final;

		VKAPI Drawable *CreateDrawable() final;
		VKAPI void SubmitDrawable(Drawable *drawable) final;

		VulkanDevice *GetVulkanDevice() const { return static_cast<VulkanDevice *>(GetDevice()); }
		VulkanInstance *GetVulkanInstance() const { return static_cast<VulkanRendererDescriptor *>(GetDescriptor())->GetInstance(); }

		VkQueue GetWorkQueue() const { return _workQueue; }
		VkResult CreateCommandBuffers(size_t count, std::vector<VkCommandBuffer> &buffers);
		VkResult CreateCommandBuffer(VkCommandBuffer &buffer);

		VkAllocationCallbacks *GetAllocatorCallback() const { return nullptr; }

		VkCommandBuffer GetGlobalCommandBuffer() const { return _commandBuffer; }
		void SubmitGlobalCommandBuffer();
		void BeginGlobalCommandBuffer();

	private:
		void RenderDrawable(VkCommandBuffer commandBuffer, VulkanDrawable *drawable);

		VulkanWindow *_mainWindow;

		PIMPL<VulkanRendererInternals> _internals;

		SpinLock _lock;

		Dictionary *_textureFormatLookup;
		Dictionary *_defaultShaders;

		VkQueue _workQueue;

		VkCommandPool _commandPool;
		VkCommandBuffer _commandBuffer;

		size_t _currentFrame;

		RNDeclareMetaAPI(VulkanRenderer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANRENDERER_H_ */

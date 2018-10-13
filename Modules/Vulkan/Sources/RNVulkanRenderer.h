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

namespace RN
{
	class VulkanWindow;
	struct VulkanRendererInternals;
	struct VulkanDrawable;
	struct VulkanRenderPass;
	class VulkanTexture;
	class VulkanCommandBuffer;
	class VulkanCommandBufferWithCallback;
	class VulkanFramebuffer;
	class VulkanStateCoordinator;
	class VulkanConstantBufferPool;
	class VulkanConstantBufferReference;

	class VulkanRenderer : public Renderer
	{
	public:
		friend VulkanFramebuffer;
		friend VulkanStateCoordinator;

		VKAPI VulkanRenderer(VulkanRendererDescriptor *descriptor, VulkanDevice *device);
		VKAPI ~VulkanRenderer();

		VKAPI Window *CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor = Window::SwapChainDescriptor()) final;
		VKAPI void SetMainWindow(Window *window) final;
		VKAPI Window *GetMainWindow() final;

		VKAPI void Render(Function &&function) final;
		VKAPI void SubmitCamera(Camera *camera, Function &&function) final;
		VKAPI void SubmitRenderPass(RenderPass *renderPass, RenderPass *previousRenderPass) final;

		VKAPI bool SupportsTextureFormat(const String *format) const final;
		VKAPI bool SupportsDrawMode(DrawMode mode) const final;

		VKAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		VKAPI size_t GetSizeForType(PrimitiveType type) const final;


		VKAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions) final;
		VKAPI GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions) final;

		VKAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file) final;
		VKAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source) final;

		VKAPI Shader *GetDefaultShader(Shader::Type type, Shader::Options *options, Shader::UsageHint usageHint = Shader::UsageHint::Default) final;
		VKAPI ShaderLibrary *GetDefaultShaderLibrary() final;

		VKAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;
		VKAPI void CreateMipMapForTexture(VulkanTexture *texture);
		VKAPI void CreateMipMaps();

		VKAPI Framebuffer *CreateFramebuffer(const Vector2 &size) final;

		VKAPI Drawable *CreateDrawable() final;
		VKAPI void DeleteDrawable(Drawable *drawable) final;
		VKAPI void SubmitDrawable(Drawable *drawable) final;
		VKAPI void SubmitLight(const Light *light) final;

		VulkanDevice *GetVulkanDevice() const { return static_cast<VulkanDevice *>(GetDevice()); }
		VulkanInstance *GetVulkanInstance() const { return static_cast<VulkanRendererDescriptor *>(GetDescriptor())->GetInstance(); }

		VkQueue GetWorkQueue() const { return _workQueue; }
		VkAllocationCallbacks *GetAllocatorCallback() const { return nullptr; }
		VkDescriptorPool GetDescriptorPool() const { return _descriptorPool; }

		VKAPI VulkanCommandBuffer *GetCommandBuffer();
		VKAPI VulkanCommandBufferWithCallback *GetCommandBufferWithCallback();
		VKAPI void SubmitCommandBuffer(VulkanCommandBuffer *commandBuffer);

		VKAPI void AddFrameFinishedCallback(std::function<void()> callback, size_t frameOffset = 0);
		VKAPI VulkanConstantBufferReference *GetConstantBufferReference(size_t size, size_t index);

	private:
		void UpdateDescriptorSets();
		void RenderDrawable(VkCommandBuffer commandBuffer, VulkanDrawable *drawable);
		void FillUniformBuffer(VulkanConstantBufferReference *constantBufferReference, VulkanDrawable *drawable, Shader *shader);

		void RenderAPIRenderPass(VulkanCommandBuffer *commandBuffer, const VulkanRenderPass &renderPass);

		void SetupRendertargets(VkCommandBuffer commandBuffer, const VulkanRenderPass &renderpass);
		VkRenderPass GetVulkanRenderPass(VulkanFramebuffer *framebuffer, VulkanFramebuffer *resolveFramebuffer, RenderPass::Flags flags);

		void CreateVulkanCommandBuffers(size_t count, std::vector<VkCommandBuffer> &buffers);
		VkCommandBuffer CreateVulkanCommandBuffer();

		void UpdateFrameFences();
		void ReleaseFrameResources(uint32 frame);

		Window *_mainWindow;
		ShaderLibrary *_defaultShaderLibrary;

		PIMPL<VulkanRendererInternals> _internals;

		Lockable _lock;

		Array *_mipMapTextures;

		VkDescriptorPool _descriptorPool;
		VkQueue _workQueue;
		VulkanCommandBuffer *_currentCommandBuffer;
		VkCommandPool _commandPool;
		Array *_submittedCommandBuffers;
		Array *_executedCommandBuffers;
		Array *_commandBufferPool;

		VulkanConstantBufferPool *_constantBufferPool;

		std::vector<VkFence> _frameFences;
		std::vector<uint32> _frameFenceValues;
		uint32 _currentFrameFenceIndex;

		size_t _currentDrawableIndex;
		size_t _currentFrame;
		size_t _completedFrame;

		RNDeclareMetaAPI(VulkanRenderer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANRENDERER_H_ */

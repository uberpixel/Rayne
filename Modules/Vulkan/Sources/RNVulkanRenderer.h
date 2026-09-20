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
	struct VulkanPreparedDrawItem;
	struct VulkanFrameSubmission;
	class VulkanTexture;
	class VulkanCommandBuffer;
	class VulkanCommandBufferWithCallback;
	class VulkanFramebuffer;
	class VulkanStateCoordinator;
	class VulkanDynamicBufferPool;
	class VulkanDynamicBufferReference;
	class VulkanStaticGPUBuffer;
	class VulkanDynamicGPUBuffer;
	struct RenderPassResources;

	class VulkanRenderer : public Renderer
	{
	public:
		friend VulkanFramebuffer;
		friend VulkanStateCoordinator;
		friend VulkanStaticGPUBuffer;
		friend VulkanDynamicGPUBuffer;
		friend VulkanTexture;

		VKAPI VulkanRenderer(VulkanRendererDescriptor *descriptor, VulkanDevice *device);
		VKAPI ~VulkanRenderer();

		VKAPI Window *CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor = Window::SwapChainDescriptor(), void *hwnd = nullptr) final;
		VKAPI void SetMainWindow(Window *window) final;
		VKAPI Window *GetMainWindow() final;

		VKAPI void Render(Function &&function) final;
		VKAPI void ScheduleRenderThreadWork(Function &&function) final;
		VKAPI void SynchronizeRenderThread() final;
		VKAPI void SubmitCamera(Camera *camera, Function &&function) final;

		VKAPI bool SupportsTextureFormat(const String *format) const final;
		VKAPI bool SupportsDrawMode(DrawMode mode) const final;

		VKAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		VKAPI size_t GetSizeForType(PrimitiveType type) const final;


		VKAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions, bool isStreamable) final;

		VKAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file) final;
		VKAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source) final;

		VKAPI ShaderLibrary *GetDefaultShaderLibrary() final;

		VKAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;
		VKAPI Texture *CreateTextureWithExternalMemory(const Texture::Descriptor &descriptor, const Texture::ExternalMemoryDescriptor &externalMemoryDescriptor) final;

		VKAPI Framebuffer *CreateFramebuffer(const Vector2 &size) final;
		VKAPI const std::vector<VkTilePropertiesQCOM> *GetFramebufferTileProperties(const Framebuffer *framebuffer) const;

		VKAPI Drawable *CreateDrawable() final;
		VKAPI void DeleteDrawable(Drawable *drawable) final;
		VKAPI void SubmitDrawable(Drawable *drawable, const SceneNode *node) final;
		VKAPI void SubmitDrawable(Drawable *drawable, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint16 renderGroup, uint64 sourceNodeUID = RenderFrame::InvalidSourceNodeUID, int32 renderPriority = SceneNode::RenderNormal) final;
		VKAPI void SubmitLight(const Light *light) final;
		VKAPI void WarmupDrawable(Mesh *mesh, Material *material, Camera *camera) final;

		VulkanDevice *GetVulkanDevice() const { return static_cast<VulkanDevice *>(GetDevice()); }
		VulkanInstance *GetVulkanInstance() const { return static_cast<VulkanRendererDescriptor *>(GetDescriptor())->GetInstance(); }

		VkQueue GetWorkQueue() const { return _workQueue; }
		VkAllocationCallbacks *GetAllocatorCallback() const { return nullptr; }

		VKAPI VulkanCommandBuffer *GetCommandBuffer();
		VKAPI VulkanCommandBuffer *StartResourcesCommandBuffer();
		VKAPI void EndResourcesCommandBuffer();
		VKAPI void SubmitCommandBuffer(VulkanCommandBuffer *commandBuffer);

		VKAPI void AddFrameFinishedCallback(std::function<void()> callback, size_t frameOffset = 0);
		VKAPI VulkanDynamicBufferReference *GetConstantBufferReference(size_t size, size_t index, GPUResource::UsageOptions usageOptions = GPUResource::UsageOptions::Uniform);
		VKAPI void UpdateDynamicBufferReference(VulkanDynamicBufferReference *reference, bool align);

	private:
		VKAPI String *GetBackendFrameStatistics() const final;

		void StartRenderThread();
		void StopRenderThread();
		bool IsOnRenderThread() const;
		void AssertOnSubmissionThread();
		void AssertOnRenderThread() const;
		void QueueFrameSubmission(Function &&function);
		void BuildFrameSubmission(VulkanFrameSubmission &submission, Function &&function);
		bool ConsumeRenderThreadWork();
		void RenderFrameSubmission(const VulkanFrameSubmission &submission);
		void WarmupDrawableOnRenderThread(const VulkanFrameSubmission &submission, const Mesh::DrawSnapshot &meshSnapshot, const Material::DrawSnapshot &materialSnapshot, uint64 materialSnapshotVersion);
		void SubmitCamera(VulkanFrameSubmission &submission, Camera *camera, Function &&function);
		size_t SubmitRootRenderPass(VulkanFrameSubmission &submission, Camera *camera, RenderPass *renderPass, const std::vector<Camera *> &multiviewSnapshotCameras);
		void SubmitRootFramePass(VulkanFrameSubmission &submission, Camera *camera, FramePass *framePass, const std::vector<Camera *> &multiviewSnapshotCameras);
		void SubmitFramePass(VulkanFrameSubmission &submission, Camera *camera, FramePass *framePass, VulkanRenderPass &previousRenderPass, const std::vector<Camera *> *multiviewSnapshotCameras);
		void SubmitRenderPass(VulkanFrameSubmission &submission, Camera *camera, RenderPass *renderPass, VulkanRenderPass &previousRenderPass, const std::vector<Camera *> *multiviewSnapshotCameras);
		void SubmitComputePass(VulkanFrameSubmission &submission, ComputePass *computePass, VulkanRenderPass *previousRenderPass, Camera *camera, const std::vector<Camera *> *multiviewSnapshotCameras);
		void SubmitDrawable(VulkanFrameSubmission &submission, Drawable *drawable, const SceneNode *node);
		void SubmitDrawable(VulkanFrameSubmission &submission, Drawable *drawable, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint16 renderGroup, uint64 sourceNodeUID, int32 renderPriority);
		bool PrepareRenderFrame(VulkanFrameSubmission &submission);
		void UpdateDescriptorSets(VulkanFrameSubmission &submission);
		void RenderDrawable(VkCommandBuffer commandBuffer, const VulkanPreparedDrawItem &drawItem, uint32 instanceCount);
		bool ShouldInheritViews(RenderPass::ViewMode viewMode, bool isSubpass, bool hasInheritedViewState, bool destinationSupportsViewState) const;
		bool SupportsViewState(const VulkanFramebuffer *framebuffer, uint8 multiviewLayer, uint8 multiviewCount) const;
		void FillUniformBuffer(Shader::ArgumentBuffer *argumentBuffer, VulkanDynamicBufferReference *dynamicBufferReference, const RenderFrame::DrawItem &drawItem, const Material::Properties &mergedMaterialProperties, const RenderFrame::Pass &framePass);
		void ResetDrawBindStateCache();
		void SetViewportAndScissor(VkCommandBuffer commandBuffer, const Rect &cameraRect);
		VulkanFrameSubmission &GetActiveFrameSubmission();

		void RenderAPIRenderPass(VulkanCommandBuffer *commandBuffer, const VulkanRenderPass &renderPass);
		void RenderComputePass(VulkanCommandBuffer *commandBuffer, const VulkanFrameSubmission &submission, const VulkanRenderPass &computePass);

		void SetupRendertargets(VkCommandBuffer commandBuffer, const VulkanFrameSubmission &submission, const VulkanRenderPass &renderPass);
		VkRenderPass GetVulkanRenderPass(const RenderFrame &renderFrame, const VulkanRenderPass *renderPass);
		void SubmitPendingResourceCommandBuffers();
		void CreateMipMapForTexture(VulkanTexture *texture);
		void CreateMipMaps();

		void CreateVulkanCommandBuffers(size_t count, std::vector<VkCommandBuffer> &buffers);
		VkCommandBuffer CreateVulkanCommandBuffer();

		void UpdateFrameFences();
		void ReleaseFrameResources(uint32 frame);

		Window *_mainWindow;
		ShaderLibrary *_defaultShaderLibrary;

		PIMPL<VulkanRendererInternals> _internals;
		VulkanFrameSubmission *_activeFrameSubmission;

		Lockable _lock;

		Array *_mipMapTextures;

		VkQueue _workQueue;
		VulkanCommandBuffer *_currentCommandBuffer;
		Lockable _currentResourcesCommandBufferLock;
		VulkanCommandBuffer *_currentResourcesCommandBuffer;
		VkCommandPool _commandPool;
		VkCommandPool _commandPoolSynchronised; //Used for asynchronously loading resources, recording onto the pool is synchronised with locks
		Array *_submittedCommandBuffers;
		Array *_executedCommandBuffers;
		Array *_commandBufferPool;
		Array *_commandBufferResourcesPool;

		VulkanDynamicBufferPool *_dynamicBufferPool;

		std::vector<VkFence> _frameFences;
		std::vector<uint32> _frameFenceValues;
		uint32 _currentFrameFenceIndex;

		size_t _currentFrame;
		size_t _completedFrame;

		uint8 _currentMultiviewLayer;
		uint8 _currentMultiviewCount;
		RenderPass *_currentMultiviewFallbackRenderPass;

		VulkanDrawable *_defaultPostProcessingDrawable;
		GPUBuffer *_fallbackGlobalBuffer;
		Texture *_fallbackGlobalTexture;

		RNDeclareMetaAPI(VulkanRenderer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANRENDERER_H_ */

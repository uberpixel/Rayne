//
//  RNMetalRenderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALRENDERER_H__
#define __RAYNE_METALRENDERER_H__

#include "RNMetal.h"
#include "RNMetalWindow.h"

namespace RN
{
	struct MetalRendererInternals;
	struct MetalDrawable;
	struct MetalPreparedDrawItem;
	struct MetalFrameSubmission;

	class MetalRendererDescriptor;
	class MetalDevice;
	class MetalWindow;
	class MetalTexture;
	class MetalFramebuffer;
	class MetalUniformBuffer;
	struct MetalRenderPass;
	class GPUBuffer;
	class MetalUniformBufferReference;
	class MetalUniformBufferPool;
	struct RenderPassResources;

	class MetalRenderer : public Renderer
	{
	public:
		friend class MetalTexture;
		friend class MetalWindow;

		MTLAPI MetalRenderer(MetalRendererDescriptor *descriptor, MetalDevice *device);
		MTLAPI ~MetalRenderer();

		MTLAPI Window *CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor = Window::SwapChainDescriptor(), void *hwnd = nullptr) final;
		MTLAPI Window *GetMainWindow() final;
		MTLAPI void SetMainWindow(Window *window) final;

		MTLAPI void Render(Function &&function) final;
		MTLAPI void ScheduleRenderThreadWork(Function &&function) final;
		MTLAPI void SynchronizeRenderThread() final;
		MTLAPI void SubmitCamera(Camera *camera, Function &&function) final;

		MTLAPI bool SupportsTextureFormat(const String *format) const final;
		MTLAPI bool SupportsDrawMode(DrawMode mode) const final;

		MTLAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		MTLAPI size_t GetSizeForType(PrimitiveType type) const final;

		MTLAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions, bool streameable) final;

		MTLAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file) final;
		MTLAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source) final;

		MTLAPI ShaderLibrary *GetDefaultShaderLibrary() final;

		MTLAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;
		MTLAPI Texture *CreateTextureWithExternalMemory(const Texture::Descriptor &descriptor, const Texture::ExternalMemoryDescriptor &externalMemoryDescriptor) final;
		MTLAPI Texture *CreateTextureWithDescriptorAndIOSurface(const Texture::Descriptor &descriptor, IOSurfaceRef ioSurface);

		MTLAPI Framebuffer *CreateFramebuffer(const Vector2 &size) final;

		MTLAPI Drawable *CreateDrawable() final;
		MTLAPI void DeleteDrawable(Drawable *drawable) final;
		MTLAPI void SubmitDrawable(Drawable *drawable, const SceneNode *node) final;
		MTLAPI void SubmitDrawable(Drawable *drawable, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint16 renderGroup, uint64 sourceNodeUID = RenderFrame::InvalidSourceNodeUID, int32 renderPriority = SceneNode::RenderNormal) final;
		MTLAPI void SubmitLight(const Light *light) final;
		
		MTLAPI static MTLResourceOptions MetalResourceOptionsFromOptions(GPUResource::AccessOptions options);
		
		MTLAPI MetalUniformBufferReference *GetUniformBufferReference(size_t size, size_t index);
		MTLAPI void UpdateUniformBufferReference(MetalUniformBufferReference *reference, bool align);
		
		MTLAPI id GetCommandQueue() const;

	protected:
#if RN_PLATFORM_IOS
		void SetRenderingSuspended(bool suspended) final;
#endif
		void StartRenderThread();
		void StopRenderThread();
		bool IsOnRenderThread() const;
		void AssertOnSubmissionThread();
		void AssertOnRenderThread() const;
		void QueueFrameSubmission(Function &&function);
		void BuildFrameSubmission(MetalFrameSubmission &submission, Function &&function);
		bool ConsumeRenderThreadWork();
		void RenderFrameSubmission(const MetalFrameSubmission &submission);
		void SubmitCamera(MetalFrameSubmission &submission, Camera *camera, Function &&function);
		size_t SubmitRootRenderPass(MetalFrameSubmission &submission, Camera *camera, RenderPass *renderPass);
		void SubmitRootFramePass(MetalFrameSubmission &submission, Camera *camera, FramePass *framePass);
		void SubmitFramePass(MetalFrameSubmission &submission, Camera *camera, FramePass *framePass, MetalRenderPass &previousRenderPass);
		void SubmitRenderPass(MetalFrameSubmission &submission, Camera *camera, RenderPass *renderPass, MetalRenderPass &previousRenderPass);
		void SubmitComputePass(MetalFrameSubmission &submission, ComputePass *computePass, MetalRenderPass *previousRenderPass, Camera *camera);
		void SubmitDrawable(MetalFrameSubmission &submission, Drawable *drawable, const SceneNode *node);
		void SubmitDrawable(MetalFrameSubmission &submission, Drawable *drawable, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint16 renderGroup, uint64 sourceNodeUID, int32 renderPriority);
		void PrepareRenderFrame(MetalFrameSubmission &submission);
		void RenderDrawable(const MetalPreparedDrawItem &drawItem, uint32 instanceCount, const MetalRenderPass &renderPass, const RenderFrame &renderFrame, const RenderFrame::Pass &framePass);
		void RenderAPIRenderPass(const MetalFrameSubmission &submission, const MetalRenderPass &renderPass);
		void RenderComputePass(const MetalFrameSubmission &submission, const MetalRenderPass &computePass);
		void FillUniformBuffer(Shader::ArgumentBuffer *argument, MetalUniformBufferReference *uniformBufferReference, const RenderFrame::DrawItem &drawItem, const Material::Properties &materialProperties, const RenderFrame::Pass &framePass);
		bool ShouldInheritViews(RenderPass::ViewMode viewMode, bool isSubpass, bool hasInheritedViewState, bool destinationSupportsViewState) const;
		bool SupportsViewState(const MetalFramebuffer *framebuffer, uint8 multiviewLayer, uint8 multiviewCount) const;
		MetalFrameSubmission &GetActiveFrameSubmission();
		Shader::UsageHint GetMetalShaderHint(Shader::UsageHint shaderHint) const;

		void CreateMipMapForTexture(MetalTexture *texture);
		void CreateMipMaps();

		Set *_mipMapTextures;

		PIMPL<MetalRendererInternals> _internals;
		Window *_mainWindow;
		MetalFrameSubmission *_activeFrameSubmission;
		
		MetalDrawable *_defaultPostProcessingDrawable;
		Material *_ppConvertMaterial;

		Lockable _lock;

		MetalUniformBufferPool *_uniformBufferPool;
		ShaderLibrary *_defaultShaderLibrary;
		
		uint8 _currentMultiviewLayer;
		RenderPass *_currentMultiviewFallbackRenderPass;

		RNDeclareMetaAPI(MetalRenderer, MTLAPI)
	};
}


#endif /* __RAYNE_METALRENDERER_H__ */

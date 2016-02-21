//
//  RNVulkanRenderer.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANRENDERER_H_
#define __RAYNE_VULKANRENDERER_H_

#include <Rayne.h>
#include "RNVulkanDevice.h"
#include "RNVulkanRendererDescriptor.h"
#include "RNVulkanBackBuffer.h"

namespace RN
{
	class VulkanWindow;
	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer(VulkanRendererDescriptor *descriptor, VulkanDevice *device);

		Window *CreateAWindow(const Vector2 &size, Screen *screen) override;
		Window *GetMainWindow() override;

		void RenderIntoWindow(Window *window, Function &&function) override;
		void RenderIntoCamera(Camera *camera, Function &&function) override;

		bool SupportsTextureFormat(const String *format) const override;
		bool SupportsDrawMode(DrawMode mode) const override;

		size_t GetAlignmentForType(PrimitiveType type) const override;
		size_t GetSizeForType(PrimitiveType type) const override;
		const String *GetTextureFormatName(const Texture::Format format) const override;

		GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions options) override;
		GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options) override;

		ShaderLibrary *CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options) override;
		ShaderLibrary *CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options) override;

		ShaderProgram *GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup) override;

		Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) override;

		Drawable *CreateDrawable() override;
		void SubmitDrawable(Drawable *drawable) override;

		VulkanDevice *GetVulkanDevice() { return static_cast<VulkanDevice *>(GetDevice()); }
		VulkanInstance *GetVulkanInstance() { return static_cast<VulkanRendererDescriptor *>(GetDescriptor())->GetInstance(); }

		VkQueue GetPresentQueue() const { return _presentQueue; }

	private:
		VulkanWindow *_mainWindow;

		VkQueue _gameQueue;
		VkQueue _presentQueue;

		RNDeclareMeta(VulkanRenderer)
	};
}


#endif /* __RAYNE_VULKANRENDERER_H_ */

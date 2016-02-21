//
//  RNVulkanRenderer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanRenderer.h"
#include "RNVulkanWindow.h"

namespace RN
{
	RNDefineMeta(VulkanRenderer, Renderer)

	VulkanRenderer::VulkanRenderer(VulkanRendererDescriptor *descriptor, VulkanDevice *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr)
	{
		vk::GetDeviceQueue(device->GetDevice(), device->GetGameQueue(), 0, &_gameQueue);
		vk::GetDeviceQueue(device->GetDevice(), device->GetPresentQueue(), 0, &_presentQueue);
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
		VulkanWindow *window = static_cast<VulkanWindow *>(twindow);

		window->AcquireBackBuffer();
		function();
		window->PresentBackBuffer();
	}
	void VulkanRenderer::RenderIntoCamera(Camera *camera, Function &&function)
	{
		function();
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
		return nullptr;
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

	Drawable *VulkanRenderer::CreateDrawable()
	{
		return nullptr;
	}
	void VulkanRenderer::SubmitDrawable(Drawable *drawable)
	{}
}

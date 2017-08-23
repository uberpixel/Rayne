//
//  RNOpenVRMetalSwapChain.cpp
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenVRMetalSwapChain.h"
#include "RNMetalInternals.h"
#include "RNMetalTexture.h"

namespace RN
{
	RNDefineMeta(OpenVRMetalSwapChain, MetalSwapChain)

	const uint32 OpenVRMetalSwapChain::kEyePadding = 16; //Use a padding of 16 pixels (oculus docs recommend 8, but it isn't enough)

	OpenVRMetalSwapChain::OpenVRMetalSwapChain(const Window::SwapChainDescriptor &descriptor, vr::IVRSystem *system) : _vrSystem(system)
	{
		MetalRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<MetalRenderer>();
		_descriptor = descriptor;

		uint32 recommendedWidth;
		uint32 recommendedHeight;
		_vrSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
		_size = Vector2(recommendedWidth * 2 + kEyePadding, recommendedHeight);
		
		NSDictionary *surfaceDefinition = @{
			(id)kIOSurfaceWidth: @((int)_size.x),
			(id)kIOSurfaceHeight: @((int)_size.y),
			(id)kIOSurfaceBytesPerElement: @(4),
			(id)kIOSurfaceIsGlobal: @YES
		};
		_targetSurface = IOSurfaceCreate((CFDictionaryRef)surfaceDefinition);
		
		Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(Texture::Format::BGRA8888SRGB, _size.x, _size.y, false);
		textureDescriptor.usageHint |= Texture::UsageHint::RenderTarget;
		_targetTexture = renderer->CreateTextureWithDescriptorAndIOSurface(textureDescriptor, _targetSurface)->Downcast<MetalTexture>();

		_descriptor.bufferCount = 1;
		_frameIndex = 0;
		_framebuffer = new MetalFramebuffer(_size, this, Texture::Format::BGRA8888SRGB, Texture::Format::Invalid);

		//TODO: Update every frame, maybe move to window
		vr::HmdMatrix34_t leftEyeMatrix = _vrSystem->GetEyeToHeadTransform(vr::Eye_Left);
		vr::HmdMatrix34_t rightEyeMatrix = _vrSystem->GetEyeToHeadTransform(vr::Eye_Right);
		_hmdToEyeViewOffset[0].x = leftEyeMatrix.m[0][3];
		_hmdToEyeViewOffset[0].y = leftEyeMatrix.m[1][3];
		_hmdToEyeViewOffset[0].z = leftEyeMatrix.m[2][3];
		_hmdToEyeViewOffset[1].x = rightEyeMatrix.m[0][3];
		_hmdToEyeViewOffset[1].y = rightEyeMatrix.m[1][3];
		_hmdToEyeViewOffset[1].z = rightEyeMatrix.m[2][3];
	}

	OpenVRMetalSwapChain::~OpenVRMetalSwapChain()
	{
		SafeRelease(_targetTexture);
		CFRelease(_targetSurface);
	}

	void OpenVRMetalSwapChain::ResizeSwapchain(const Vector2& size)
	{
		_size = size;
		//_framebuffer->WillUpdateSwapChain(); //As all it does is free the swap chain d3d buffer resources, it would free the targetTexture resource and should't be called in this case...
		SafeRelease(_targetTexture);
		CFRelease(_targetSurface);
		NSDictionary *surfaceDefinition = @{
			(id)kIOSurfaceWidth: @((int)_size.x),
			(id)kIOSurfaceHeight: @((int)_size.y),
			(id)kIOSurfaceBytesPerElement: @(4),
			(id)kIOSurfaceIsGlobal: @YES
		};
		_targetSurface = IOSurfaceCreate((CFDictionaryRef)surfaceDefinition);
		Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(Texture::Format::BGRA8888SRGB, _size.x, _size.y, false);
		textureDescriptor.usageHint |= Texture::UsageHint::RenderTarget;
		MetalRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<MetalRenderer>();
		_targetTexture = renderer->CreateTextureWithDescriptorAndIOSurface(textureDescriptor, _targetSurface)->Downcast<MetalTexture>();
		_framebuffer->DidUpdateSwapChain(_size, Texture::Format::BGRA8888SRGB, Texture::Format::Invalid);
	}


	void OpenVRMetalSwapChain::AcquireBackBuffer()
	{
		_frameIndex += 1;
	}

	void OpenVRMetalSwapChain::Prepare()
	{
		
	}

	void OpenVRMetalSwapChain::Finalize()
	{
		
	}

	void OpenVRMetalSwapChain::PresentBackBuffer(id<MTLCommandBuffer> commandBuffer)
	{
		vr::Texture_t eyeTexture = { _targetSurface, vr::TextureType_IOSurface, vr::ColorSpace_Gamma };

		vr::VRTextureBounds_t bounds;
		bounds.vMin = 0.0f;
		bounds.vMax = 1.0f;

		bounds.uMin = 0.0f;
		bounds.uMax = 0.5f - kEyePadding * 0.5f / _size.x;

		vr::VRCompositor()->Submit(vr::Eye_Left, &eyeTexture, &bounds, vr::Submit_Default);

		bounds.uMin = 0.5f + kEyePadding * 0.5f / _size.x;
		bounds.uMax = 1.0f;
		vr::VRCompositor()->Submit(vr::Eye_Right, &eyeTexture, &bounds, vr::Submit_Default);
	}
	
	void OpenVRMetalSwapChain::PostPresent(id<MTLCommandBuffer> commandBuffer)
	{
		[commandBuffer waitUntilScheduled];
		vr::VRCompositor()->PostPresentHandoff();
	}
	
	id OpenVRMetalSwapChain::GetMTLTexture() const
	{
		return static_cast<id>(_targetTexture->__GetUnderlyingTexture());
	}

	void OpenVRMetalSwapChain::UpdatePredictedPose()
	{
		vr::VRCompositor()->WaitGetPoses(_frameDevicePose, vr::k_unMaxTrackedDeviceCount, _predictedDevicePose, vr::k_unMaxTrackedDeviceCount);
	}
}

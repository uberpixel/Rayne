//
//  RNOpenVRD3D12SwapChain.cpp
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenVRD3D12SwapChain.h"

#include "RND3D12Internals.h"
#include "RND3D12Renderer.h"
#include "RND3D12Framebuffer.h"

namespace RN
{
	RNDefineMeta(OpenVRD3D12SwapChain, D3D12SwapChain)

	OpenVRD3D12SwapChain::OpenVRD3D12SwapChain(const Window::SwapChainDescriptor &descriptor, vr::IVRSystem *system) : OpenVRSwapChain(system), _isFirstRender(true)
	{
		_renderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
		_descriptor = descriptor;
		_descriptor.colorFormat = Texture::Format::RGBA8888SRGB; //OpenVR expects RGBA!

		uint32 recommendedWidth;
		uint32 recommendedHeight;
		_vrSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
		_size = Vector2(recommendedWidth * 2 + kEyePadding, recommendedHeight);

		Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(_descriptor.colorFormat, _size.x, _size.y, false);
		textureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
		_targetTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);

		_descriptor.bufferCount = 1;
		_frameIndex = 0;
		_framebuffer = new D3D12Framebuffer(_size, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);

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

	OpenVRD3D12SwapChain::~OpenVRD3D12SwapChain()
	{
		SafeRelease(_targetTexture);
	}

	void OpenVRD3D12SwapChain::ResizeOpenVRSwapChain(const Vector2& size)
	{
		_size = size;
		//_framebuffer->WillUpdateSwapChain(); //As all it does is free the swap chain d3d buffer resources, it would free the targetTexture resource and should't be called in this case...
		SafeRelease(_targetTexture);
		Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(_descriptor.colorFormat, _size.x, _size.y, false);
		textureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
		_targetTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);
		_framebuffer->DidUpdateSwapChain(_size, _descriptor.colorFormat, _descriptor.depthStencilFormat);
		_isFirstRender = true;
	}


	void OpenVRD3D12SwapChain::AcquireBackBuffer()
	{
		
	}

	void OpenVRD3D12SwapChain::Prepare(D3D12CommandList *commandList)
	{
		if (_isFirstRender)
			return;

		_targetTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	void OpenVRD3D12SwapChain::Finalize(D3D12CommandList *commandList)
	{
		_targetTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		_isFirstRender = false;
	}

	void OpenVRD3D12SwapChain::PresentBackBuffer()
	{
		vr::D3D12TextureData_t d3d12EyeTexture;
		d3d12EyeTexture.m_pResource = _targetTexture->Downcast<D3D12Texture>()->GetD3D12Resource();
		d3d12EyeTexture.m_pCommandQueue = _renderer->GetCommandQueue();
		d3d12EyeTexture.m_nNodeMask = 0;
		vr::Texture_t eyeTexture = { (void *)&d3d12EyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma };

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

	ID3D12Resource *OpenVRD3D12SwapChain::GetD3D12ColorBuffer(int i) const
	{
		return _targetTexture->Downcast<D3D12Texture>()->GetD3D12Resource();
	}

	Framebuffer *OpenVRD3D12SwapChain::GetOpenVRSwapChainFramebuffer() const
	{
		return GetFramebuffer()->Downcast<Framebuffer>();
	}
}

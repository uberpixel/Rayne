//
//  RNOpenXRCompositorLayer.cpp
//  Rayne-VR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenXRCompositorLayer.h"
#include "RNOpenXRInternals.h"

namespace RN
{
	RNDefineMeta(OpenXRCompositorLayer, VRCompositorLayer)

	OpenXRCompositorLayer::OpenXRCompositorLayer(Type type, const Window::SwapChainDescriptor &descriptor, Vector2 resolution, bool supportFoveation, OpenXRWindow *window) : VRCompositorLayer(type), _swapChain(nullptr), _isActive(true), _isSessionActive(false)
	{
		Window::SwapChainDescriptor tempDescriptor = descriptor;

		if(type == TypeProjectionView) tempDescriptor.layerCount = 2; //Force two layers here, one for each view
		else if(type == TypeQuad) tempDescriptor.layerCount = 1; //Force one layer here
		
		if(type != TypePassthrough)
		{
#ifdef XR_USE_GRAPHICS_API_VULKAN
			if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("Vulkan")))
			{
				_swapChain = new OpenXRVulkanSwapChain(window, tempDescriptor, resolution, supportFoveation);
			}
#endif
#ifdef XR_USE_GRAPHICS_API_D3D12
			if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("D3D12")))
			{
				_swapChain = new OpenXRD3D12SwapChain(window, tempDescriptor, resolution);
			}
#endif
		}
	}

	OpenXRCompositorLayer::~OpenXRCompositorLayer()
	{
		if(_swapChain)
		{
#ifdef XR_USE_GRAPHICS_API_D3D12
			if(_swapChain->_swapChainType == OpenXRSwapChain::SwapChainType::D3D12)
			{
				OpenXRD3D12SwapChain *swapChain = static_cast<OpenXRD3D12SwapChain*>(_swapChain);
				swapChain->Release();
				_swapChain = nullptr;
			}
#endif

#ifdef XR_USE_GRAPHICS_API_VULKAN
			if(_swapChain->_swapChainType == OpenXRSwapChain::SwapChainType::Vulkan)
			{
				OpenXRVulkanSwapChain *swapChain = static_cast<OpenXRVulkanSwapChain*>(_swapChain);
				swapChain->Release();
				_swapChain = nullptr;
				return;
			}
#endif
		}
	}

	void OpenXRCompositorLayer::SetActive(bool active)
	{
		_isActive = active;

		if(!_swapChain) return;
		_swapChain->SetActive(_isSessionActive && _isActive);
	}

	void OpenXRCompositorLayer::SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic)
	{
		_swapChain->SetFixedFoveatedRenderingLevel(level, dynamic);
	}

	Vector2 OpenXRCompositorLayer::GetSize() const
	{
		return _swapChain->GetSwapChainSize();
	}

	size_t OpenXRCompositorLayer::GetImageCount() const
	{
		if(!_swapChain) return 0;

		if(_type == TypeProjectionView) return 2;
		else return 1;
	}

	Framebuffer *OpenXRCompositorLayer::GetFramebuffer() const
	{
		if(!_swapChain) return nullptr;
		return _swapChain->GetSwapChainFramebuffer();
	}

	Framebuffer *OpenXRCompositorLayer::GetFramebuffer(uint8 eye) const
	{
		if(!_swapChain) return nullptr;

		RN_ASSERT(eye < 2, "Eye Index needs to be 0 or 1");
		return _swapChain->GetSwapChainFramebuffer();
	}

	void OpenXRCompositorLayer::SetSessionActive(bool active)
	{
		_isSessionActive = active;

		if(!_swapChain) return;
		_swapChain->SetActive(_isSessionActive && _isActive);
	}
}

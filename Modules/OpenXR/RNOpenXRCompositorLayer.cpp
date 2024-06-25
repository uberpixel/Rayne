//
//  RNOpenXRCompositorLayer.cpp
//  Rayne-VR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenXRCompositorLayer.h"
#include "RNOpenXRInternals.h"
#include "RNOpenXRWindow.h"

namespace RN
{
	RNDefineMeta(OpenXRCompositorLayer, VRCompositorLayer)

	OpenXRCompositorLayer::OpenXRCompositorLayer(Type type, const Window::SwapChainDescriptor &descriptor, Vector2 resolution, bool supportFoveation, OpenXRWindow *window) : VRCompositorLayer(type), _internals(new OpenXRCompositorLayerInternals()), _swapChain(nullptr), _isActive(true), _isSessionActive(false)
	{
		Window::SwapChainDescriptor tempDescriptor = descriptor;

		if(type == TypeProjectionView) tempDescriptor.layerCount = 2; //Force two layers here, one for each view
		else if(type == TypeQuad) tempDescriptor.layerCount = 1; //Force one layer here
		
		if(type != TypePassthrough)
		{
#ifdef XR_USE_GRAPHICS_API_VULKAN
			if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("Vulkan")))
			{
				_swapChain = new OpenXRVulkanSwapChain(window, this, tempDescriptor, resolution, supportFoveation);
			}
#endif
#ifdef XR_USE_GRAPHICS_API_D3D12
			if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("D3D12")))
			{
				_swapChain = new OpenXRD3D12SwapChain(window, tempDescriptor, resolution);
			}
#endif
		}

		_internals->layerSettings.type = XR_TYPE_COMPOSITION_LAYER_SETTINGS_FB;
		_internals->layerSettings.next = nullptr;
		_internals->layerSettings.layerFlags = XR_COMPOSITION_LAYER_SETTINGS_NORMAL_SUPER_SAMPLING_BIT_FB | XR_COMPOSITION_LAYER_SETTINGS_NORMAL_SHARPENING_BIT_FB | XR_COMPOSITION_LAYER_SETTINGS_AUTO_LAYER_FILTER_BIT_META;

		if(type == TypeProjectionView)
		{
			_internals->layerProjectionViews[0].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
			_internals->layerProjectionViews[0].next = nullptr;
			_internals->layerProjectionViews[0].pose = window->_internals->views[0].pose;
			_internals->layerProjectionViews[0].fov = window->_internals->views[0].fov;
			_internals->layerProjectionViews[0].subImage.swapchain = _swapChain->_internals->swapchain;
			_internals->layerProjectionViews[0].subImage.imageRect.offset.x = 0;
			_internals->layerProjectionViews[0].subImage.imageRect.offset.y = 0;
			_internals->layerProjectionViews[0].subImage.imageRect.extent.width = _swapChain->GetSwapChainSize().x;
			_internals->layerProjectionViews[0].subImage.imageRect.extent.height = _swapChain->GetSwapChainSize().y;
			_internals->layerProjectionViews[0].subImage.imageArrayIndex = 0;

			_internals->layerProjectionViews[1].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
			_internals->layerProjectionViews[1].next = nullptr;
			_internals->layerProjectionViews[1].pose = window->_internals->views[1].pose;
			_internals->layerProjectionViews[1].fov = window->_internals->views[1].fov;
			_internals->layerProjectionViews[1].subImage.swapchain = _swapChain->_internals->swapchain;
			_internals->layerProjectionViews[1].subImage.imageRect.offset.x = 0;
			_internals->layerProjectionViews[1].subImage.imageRect.offset.y = 0;
			_internals->layerProjectionViews[1].subImage.imageRect.extent.width = _swapChain->GetSwapChainSize().x;
			_internals->layerProjectionViews[1].subImage.imageRect.extent.height = _swapChain->GetSwapChainSize().y;
			_internals->layerProjectionViews[1].subImage.imageArrayIndex = 1;

			_internals->layerProjection.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
			_internals->layerProjection.next = nullptr;
			_internals->layerProjection.layerFlags = XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT | XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
			_internals->layerProjection.space = window->_internals->trackingSpace;
			_internals->layerProjection.viewCount = 2;
			_internals->layerProjection.views = _internals->layerProjectionViews;

			_internals->layerBaseHeader = reinterpret_cast<XrCompositionLayerBaseHeader*>(&_internals->layerProjection);
		}
		else if(type == TypeQuad)
		{
			_internals->layerQuad.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
			_internals->layerQuad.next = nullptr;//_supportsCompositionLayerSettings? &layerSettings : nullptr;
			_internals->layerQuad.layerFlags = 0;//XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
			_internals->layerQuad.space = window->_internals->trackingSpace;
			_internals->layerQuad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
			_internals->layerQuad.subImage.swapchain = _swapChain->_internals->swapchain;
			_internals->layerQuad.subImage.imageRect.offset.x = 0;
			_internals->layerQuad.subImage.imageRect.offset.y = 0;
			_internals->layerQuad.subImage.imageRect.extent.width = _swapChain->GetSwapChainSize().x;
			_internals->layerQuad.subImage.imageRect.extent.height = _swapChain->GetSwapChainSize().y;
			_internals->layerQuad.subImage.imageArrayIndex = 0;
			_internals->layerQuad.pose.position.x = _position.x;
			_internals->layerQuad.pose.position.y = _position.y;
			_internals->layerQuad.pose.position.z = _position.z;
			_internals->layerQuad.pose.orientation.x = _rotation.x;
			_internals->layerQuad.pose.orientation.y = _rotation.y;
			_internals->layerQuad.pose.orientation.z = _rotation.z;
			_internals->layerQuad.pose.orientation.w = _rotation.w;
			_internals->layerQuad.size.width = _rotation.x;
			_internals->layerQuad.size.height = _rotation.y;

			_internals->layerBaseHeader = reinterpret_cast<XrCompositionLayerBaseHeader*>(&_internals->layerQuad);
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

	void OpenXRCompositorLayer::UpdateForCurrentFrame(const OpenXRWindow *window)
	{
		if(_type == TypeProjectionView)
		{
			_internals->layerProjectionViews[0].pose = window->_internals->views[0].pose;
			_internals->layerProjectionViews[0].fov = window->_internals->views[0].fov;
			_internals->layerProjectionViews[0].subImage.swapchain = _swapChain->_internals->swapchain;
			_internals->layerProjectionViews[0].subImage.imageRect.offset.x = 0;
			_internals->layerProjectionViews[0].subImage.imageRect.offset.y = 0;
			_internals->layerProjectionViews[0].subImage.imageRect.extent.width = _swapChain->GetSwapChainSize().x;
			_internals->layerProjectionViews[0].subImage.imageRect.extent.height = _swapChain->GetSwapChainSize().y;
			_internals->layerProjectionViews[0].subImage.imageArrayIndex = 0;

			_internals->layerProjectionViews[1].pose = window->_internals->views[1].pose;
			_internals->layerProjectionViews[1].fov = window->_internals->views[1].fov;
			_internals->layerProjectionViews[1].subImage.swapchain = _swapChain->_internals->swapchain;
			_internals->layerProjectionViews[1].subImage.imageRect.offset.x = 0;
			_internals->layerProjectionViews[1].subImage.imageRect.offset.y = 0;
			_internals->layerProjectionViews[1].subImage.imageRect.extent.width = _swapChain->GetSwapChainSize().x;
			_internals->layerProjectionViews[1].subImage.imageRect.extent.height = _swapChain->GetSwapChainSize().y;
			_internals->layerProjectionViews[1].subImage.imageArrayIndex = 1;

			_internals->layerProjection.space = window->_internals->trackingSpace;
		}
		else if(_type == TypeQuad)
		{
			_internals->layerQuad.space = window->_internals->trackingSpace;
			_internals->layerQuad.subImage.swapchain = _swapChain->_internals->swapchain;
			_internals->layerQuad.subImage.imageRect.offset.x = 0;
			_internals->layerQuad.subImage.imageRect.offset.y = 0;
			_internals->layerQuad.subImage.imageRect.extent.width = _swapChain->GetSwapChainSize().x;
			_internals->layerQuad.subImage.imageRect.extent.height = _swapChain->GetSwapChainSize().y;
			_internals->layerQuad.subImage.imageArrayIndex = 0;
			_internals->layerQuad.pose.position.x = _position.x;
			_internals->layerQuad.pose.position.y = _position.y;
			_internals->layerQuad.pose.position.z = _position.z;
			_internals->layerQuad.pose.orientation.x = _rotation.x;
			_internals->layerQuad.pose.orientation.y = _rotation.y;
			_internals->layerQuad.pose.orientation.z = _rotation.z;
			_internals->layerQuad.pose.orientation.w = _rotation.w;
			_internals->layerQuad.size.width = _rotation.x;
			_internals->layerQuad.size.height = _rotation.y;
		}

		if(window->_supportsDynamicResolution)
		{
			XrRecommendedLayerResolutionGetInfoMETA recommendedLayerResolutionGetInfo;
			recommendedLayerResolutionGetInfo.type = XR_TYPE_RECOMMENDED_LAYER_RESOLUTION_GET_INFO_META;
			recommendedLayerResolutionGetInfo.next = nullptr;
			recommendedLayerResolutionGetInfo.layer = _internals->layerBaseHeader;
			recommendedLayerResolutionGetInfo.predictedDisplayTime = window->_internals->predictedDisplayTime;
			XrRecommendedLayerResolutionMETA recommendedLayerResolution;
			if(!XR_FAILED(window->_internals->GetRecommendedLayerResolutionMETA(window->_internals->session, &recommendedLayerResolutionGetInfo, &recommendedLayerResolution)))
			{
				if(recommendedLayerResolution.isValid)
				{
					if(_type == TypeProjectionView)
					{
						_internals->layerProjectionViews[0].subImage.imageRect.extent.width = recommendedLayerResolution.recommendedImageDimensions.width;
						_internals->layerProjectionViews[0].subImage.imageRect.extent.height = recommendedLayerResolution.recommendedImageDimensions.height;

						_internals->layerProjectionViews[1].subImage.imageRect.extent.width = recommendedLayerResolution.recommendedImageDimensions.width;
						_internals->layerProjectionViews[1].subImage.imageRect.extent.height = recommendedLayerResolution.recommendedImageDimensions.height;
					}
					else if(_type == TypeQuad)
					{
						_internals->layerQuad.subImage.imageRect.extent.width = recommendedLayerResolution.recommendedImageDimensions.width;
						_internals->layerQuad.subImage.imageRect.extent.height = recommendedLayerResolution.recommendedImageDimensions.height;
					}

					//TODO: Need to also adapt the framebuffer size!
					//RNDebug("new recommended resolution: " << recommendedLayerResolution.recommendedImageDimensions.width << " x " << recommendedLayerResolution.recommendedImageDimensions.height);
					_swapChain->GetSwapChainFramebuffer()->SetSize(RN::Vector2(recommendedLayerResolution.recommendedImageDimensions.width, recommendedLayerResolution.recommendedImageDimensions.height));
				}
			}
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

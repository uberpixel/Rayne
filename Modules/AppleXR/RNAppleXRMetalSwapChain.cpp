//
//  RNAppleXRMetalSwapChain.cpp
//  Rayne-AppleXR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAppleXRMetalSwapChain.h"
#include "RNMetalInternals.h"
#include "RNMetalTexture.h"

namespace RN
{
	RNDefineMeta(AppleXRMetalSwapChain, MetalSwapChain)

	AppleXRMetalSwapChain::AppleXRMetalSwapChain(const Window::SwapChainDescriptor &descriptor, cp_layer_renderer_t layerRenderer) : AppleXRSwapChain(layerRenderer), _drawable(nullptr)
	{
		MetalRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<MetalRenderer>();
		_descriptor = descriptor;
		
		_descriptor.depthStencilFormat = Texture::Format::Depth_32F;
		
		cp_layer_renderer_configuration_t configuration = cp_layer_renderer_get_configuration(layerRenderer);
		cp_layer_renderer_layout layout = cp_layer_renderer_configuration_get_layout(configuration);
		cp_layer_renderer_properties_t properties = cp_layer_renderer_get_properties(layerRenderer);
		
		size_t textureTopologyCount = cp_layer_renderer_properties_get_texture_topology_count(properties);
		for(size_t i = 0; i < textureTopologyCount; i++)
		{
			cp_texture_topology_t textureTopology = cp_layer_renderer_properties_get_texture_topology(properties, 0);
			MTLTextureType textureType = cp_texture_topology_get_texture_type(textureTopology);
		}
		
/*		size_t viewCount = cp_layer_renderer_properties_get_view_count(properties);
		for(size_t i = 0; i < viewCount; i++)
		{
			cp_get_view
			cp_view_get_view_texture_map(<#cp_view_t  _Nonnull view#>)
			cp_view_texture_map_get_viewport(<#cp_view_texture_map_t  _Nonnull view_texture_map#>)
		}
		
		
		cp_layer_renderer_layout layout = cp_layer_renderer_configuration_get(configuration);*/
		
		_size = Vector2(1024.0f, 1024.0f); //Size is hopefully not relevant for anything
		
		
		_frameIndex = 0;
		_framebuffer = new MetalFramebuffer(_size, this, Texture::Format::BGRA_8_SRGB, Texture::Format::Invalid);
		
		Framebuffer::TargetView targetView;
		targetView.mipmap = 0;
		targetView.slice = 0;
		targetView.length = 1;
		targetView.texture = nullptr;
		_framebuffer->SetSwapchainDepthStencilTarget(targetView, _descriptor.depthStencilFormat);

		//TODO: Update every frame, maybe move to window
/*		vr::HmdMatrix34_t leftEyeMatrix = _vrSystem->GetEyeToHeadTransform(vr::Eye_Left);
		vr::HmdMatrix34_t rightEyeMatrix = _vrSystem->GetEyeToHeadTransform(vr::Eye_Right);
		_hmdToEyeViewOffset[0].x = leftEyeMatrix.m[0][3];
		_hmdToEyeViewOffset[0].y = leftEyeMatrix.m[1][3];
		_hmdToEyeViewOffset[0].z = leftEyeMatrix.m[2][3];
		_hmdToEyeViewOffset[1].x = rightEyeMatrix.m[0][3];
		_hmdToEyeViewOffset[1].y = rightEyeMatrix.m[1][3];
		_hmdToEyeViewOffset[1].z = rightEyeMatrix.m[2][3];*/
	}

	AppleXRMetalSwapChain::~AppleXRMetalSwapChain()
	{
		
	}


	void AppleXRMetalSwapChain::AcquireBackBuffer()
	{
		if(!isActive || !_frame) return;
		
		cp_frame_end_update(_frame);

		// Wait until the optimal time for querying the input
		cp_time_wait_until(cp_frame_timing_get_optimal_input_time(_predictedTime));

		cp_frame_start_submission(_frame);

		_drawable = cp_frame_query_drawable(_frame);
		if(_drawable == nullptr) return;

		cp_frame_timing_t final_timing = cp_drawable_get_frame_timing(_drawable);
	
		cp_drawable_set_device_anchor(_drawable, _worldAnchor);
	
		//ar_pose_t pose;// = my_engine_get_ar_pose(engine, final_timing);
		//cp_drawable_set_ar_pose(drawable, pose);
	}

	void AppleXRMetalSwapChain::Prepare()
	{
		
	}

	void AppleXRMetalSwapChain::Finalize()
	{
		
	}

	void AppleXRMetalSwapChain::PresentBackBuffer(id<MTLCommandBuffer> commandBuffer)
	{
		if(!isActive || !_frame || !_drawable) return;
		
		cp_drawable_encode_present(_drawable, commandBuffer);
	}
	
	void AppleXRMetalSwapChain::PostPresent(id<MTLCommandBuffer> commandBuffer)
	{
		if(!isActive || !_frame || !_drawable) return;
		
		cp_frame_end_submission(_frame);
	}
	
	id AppleXRMetalSwapChain::GetMetalColorTexture() const
	{
		if(!_drawable) return nullptr;
		
		return cp_drawable_get_color_texture(_drawable, 0);
	}

	id AppleXRMetalSwapChain::GetMetalDepthTexture() const
	{
		if(!_drawable) return nullptr;
		
		return cp_drawable_get_depth_texture(_drawable, 0);
	}
	
	Framebuffer *AppleXRMetalSwapChain::GetAppleXRSwapChainFramebuffer() const
	{
		return GetFramebuffer()->Downcast<Framebuffer>();
	}
}

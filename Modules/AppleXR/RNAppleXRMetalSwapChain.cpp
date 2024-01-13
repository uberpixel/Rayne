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
		
		_size = Vector2(1024.0f, 1024.0f); //Will be set to the correct size each frame
		
		
		_frameIndex = 0;
		_framebuffer = new MetalFramebuffer(_size, this, Texture::Format::BGRA_8_SRGB, Texture::Format::Invalid);
		
		Framebuffer::TargetView targetView;
		targetView.mipmap = 0;
		targetView.slice = 0;
		targetView.length = 1;
		targetView.texture = nullptr;
		_framebuffer->SetSwapchainDepthStencilTarget(targetView, _descriptor.depthStencilFormat);
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
		
		size_t viewCount = cp_drawable_get_view_count(_drawable);
		for(int i = 0; i < viewCount; i++)
		{
			cp_view_t view = cp_drawable_get_view(_drawable, i);
			cp_view_texture_map_t textureMap = cp_view_get_view_texture_map(view);
			MTLViewport viewPort = cp_view_texture_map_get_viewport(textureMap);
			_size.x = viewPort.width;
			_size.y = viewPort.height;
			_framebuffer->DidUpdateSwapChainSize(_size);
			
			simd_float4 tangents = cp_view_get_tangents(view);
			simd_float2 depth_range = cp_drawable_get_depth_range(_drawable);
			
			simd_float4x4 eye_transform = cp_view_get_transform(view);
			_hmdToEyeViewOffset[i].x = eye_transform.columns[3][1];
			_hmdToEyeViewOffset[i].y = eye_transform.columns[3][2];
			_hmdToEyeViewOffset[i].z = eye_transform.columns[3][3];
			
			_hmdEyeProjectionMatrix[i] = RN::Matrix::WithProjectionPerspective(tangents[0], tangents[1], tangents[2], tangents[3], depth_range[1], depth_range[0]);
		}

		//cp_frame_timing_t final_timing = cp_drawable_get_frame_timing(_drawable);
		//ar_pose_t pose;// = my_engine_get_ar_pose(engine, final_timing);
		//cp_drawable_set_ar_pose(drawable, pose);
	
		cp_drawable_set_device_anchor(_drawable, _worldAnchor);
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

//
//  RNMetalInternals.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALINTERNALS_H__
#define __RAYNE_METALINTERNALS_H__

#import <Metal/Metal.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#include "RNMetal.h"
#include "RNMetalStateCoordinator.h"
#include "RNMetalUniformBuffer.h"
#include "RNMetalFramebuffer.h"
#include "RNMetalRenderer.h"

@interface RNMetalView : NSView
- (id<CAMetalDrawable>)nextDrawable;
- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device andFormat:(MTLPixelFormat)format;
- (CGSize)getSize;
@end

@interface RNMetalWindow : NSWindow
@end

namespace RN
{
	class MetalWindow;
	class Framebuffer;
	class Camera;
	class MetalGPUBuffer;
	class MetalTexture;

	struct MetalDrawable : public Drawable
	{
		//TODO: Maybe store these per camera/renderpass!?
		struct CameraSpecific
		{
			const MetalRenderingState *pipelineState;
			MetalUniformBufferReference *vertexBuffer;
			MetalUniformBufferReference *fragmentBuffer;
			bool dirty;
		};

		~MetalDrawable()
		{
			for(CameraSpecific specific : _cameraSpecifics)
			{
				if(specific.vertexBuffer)
					delete specific.vertexBuffer;

				if(specific.fragmentBuffer)
					delete specific.fragmentBuffer;
			}
		}

		void AddCameraSepecificsIfNeeded(size_t cameraID)
		{
			while(_cameraSpecifics.size() <= cameraID)
			{
				_cameraSpecifics.push_back({nullptr, nullptr, nullptr, true});
			}
		}

		void UpdateRenderingState(size_t cameraID, Renderer *renderer, const MetalRenderingState *state)
		{
			_cameraSpecifics[cameraID].pipelineState = state;

			if(_cameraSpecifics[cameraID].vertexBuffer)
				delete _cameraSpecifics[cameraID].vertexBuffer;

			if(_cameraSpecifics[cameraID].fragmentBuffer)
				delete _cameraSpecifics[cameraID].fragmentBuffer;

			MetalRenderer *metalRenderer = renderer->Downcast<MetalRenderer>();
			
			//TODO: Support multiple uniform buffers
			size_t totalSize = state->vertexShader->GetSignature()->GetTotalUniformSize();
			if(totalSize > 0)
				_cameraSpecifics[cameraID].vertexBuffer = metalRenderer->GetUniformBufferReference(totalSize, 1)->Retain();

			totalSize = state->fragmentShader->GetSignature()->GetTotalUniformSize();
			if(totalSize > 0)
				_cameraSpecifics[cameraID].fragmentBuffer = metalRenderer->GetUniformBufferReference(totalSize, 2)->Retain();
			
			_cameraSpecifics[cameraID].dirty = false;
		}
		
		virtual void MakeDirty() override
		{
			for(int i = 0; i < _cameraSpecifics.size(); i++)
			{
				_cameraSpecifics[i].dirty = true;
			}
		}

		std::vector<CameraSpecific> _cameraSpecifics;
	};

	struct MetalPointLight
	{
		Vector3 position;
		float range;
		Color color;
	};

	struct MetalSpotLight
	{
		Vector3 position;
		float range;
		Vector3 direction;
		float angle;
		Color color;
	};

	struct MetalDirectionalLight
	{
		Vector3 direction;
		Color color;
	};

	struct MetalRenderPass
	{
		enum Type
		{
			Default,
			ResolveMSAA,
			Blit,
			Convert
		};

		Type type;
		RenderPass *renderPass;
		RenderPass *previousRenderPass;

		MetalFramebuffer *framebuffer;
		Shader::UsageHint shaderHint;
		Material *overrideMaterial;
		MetalFramebuffer *resolveFramebuffer;

		Vector3 viewPosition;
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;
		
		Color cameraAmbientColor;

		std::vector<MetalDrawable *> drawables;

		std::vector<MetalPointLight> pointLights;
		std::vector<MetalSpotLight> spotLights;
		std::vector<MetalDirectionalLight> directionalLights;

		std::vector<Matrix> directionalShadowMatrices;
		MetalTexture *directionalShadowDepthTexture;
		Vector2 directionalShadowInfo;
	};


	struct MetalRendererInternals
	{
		std::vector<MetalRenderPass> renderPasses;
		MetalStateCoordinator stateCoordinator;

		id<MTLDevice> device;
		id<MTLCommandQueue> commandQueue;

		id<MTLCommandBuffer> commandBuffer;
		id<MTLRenderCommandEncoder> commandEncoder;

		std::vector<MetalSwapChain *>swapChains;

		size_t currentRenderPassIndex;
		const MetalRenderingState *currentRenderState;
	};

	struct MetalWindowInternals
	{
		NSWindow *window;
	};
}

#endif /* __RAYNE_METALINTERNALS_H__ */

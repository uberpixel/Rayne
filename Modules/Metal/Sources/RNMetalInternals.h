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

#include "RNMetal.h"
#include "RNMetalStateCoordinator.h"
#include "RNMetalUniformBuffer.h"
#include "RNMetalFramebuffer.h"
#include "RNMetalRenderer.h"

#if RN_PLATFORM_MAC_OS
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#endif

#if RN_PLATFORM_IOS
#import <UIKit/UIKit.h>
#endif

#if RN_PLATFORM_MAC_OS
@interface RNMetalView : NSView
- (id<CAMetalDrawable>)nextDrawable;
- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device screen:(RN::Screen*)screen andFormat:(MTLPixelFormat)format;
- (CGSize)getSize;
@end

@interface RNMetalWindow : NSWindow
@end
#endif

#if RN_PLATFORM_IOS
class RNMetalLayerContainer
{
public:
	RNMetalLayerContainer(CAMetalLayer *metalLayer);
	id<CAMetalDrawable> GetNextDrawable();
	RN::Vector2 GetSize();
	
private:
	CAMetalLayer *_metalLayer;
};
#endif

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
			Camera *camera;
			const MetalRenderingState *pipelineState;
			bool dirty;
			
			std::vector<Shader::ArgumentBuffer*> argumentBufferToUniformBufferMapping;
			std::vector<MetalUniformBufferReference*> vertexShaderUniformBuffers;
			std::vector<MetalUniformBufferReference*> fragmentShaderUniformBuffers;
		};

		~MetalDrawable()
		{
			for(CameraSpecific specific : _cameraSpecifics)
			{
				for(MetalUniformBufferReference *buffer : specific.vertexShaderUniformBuffers)
					delete buffer;
				
				for(MetalUniformBufferReference *buffer : specific.fragmentShaderUniformBuffers)
					delete buffer;
				
				for(Shader::ArgumentBuffer *buffer : specific.argumentBufferToUniformBufferMapping)
					buffer->Release();
			}
		}

		void AddCameraSepecificsIfNeeded(size_t cameraID)
		{
			while(_cameraSpecifics.size() <= cameraID)
			{
				_cameraSpecifics.push_back({nullptr, nullptr, true});
			}
		}

		void UpdateRenderingState(size_t cameraID, Camera * camera, Renderer *renderer, const MetalRenderingState *state)
		{
			_cameraSpecifics[cameraID].camera = camera;
			_cameraSpecifics[cameraID].pipelineState = state;
			
			for(Shader::ArgumentBuffer *buffer : _cameraSpecifics[cameraID].argumentBufferToUniformBufferMapping)
				buffer->Release();
			_cameraSpecifics[cameraID].argumentBufferToUniformBufferMapping.clear();
			
			for(MetalUniformBufferReference *buffer : _cameraSpecifics[cameraID].vertexShaderUniformBuffers)
				buffer->Release();
			_cameraSpecifics[cameraID].vertexShaderUniformBuffers.clear();
			
			for(MetalUniformBufferReference *buffer : _cameraSpecifics[cameraID].fragmentShaderUniformBuffers)
				buffer->Release();
			_cameraSpecifics[cameraID].fragmentShaderUniformBuffers.clear();

			MetalRenderer *metalRenderer = renderer->Downcast<MetalRenderer>();
			
			const Shader::Signature *vertexShaderSignature = state->vertexShader->GetSignature();
			vertexShaderSignature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				size_t totalSize = buffer->GetTotalUniformSize();
				if(totalSize > 0)
				{
					_cameraSpecifics[cameraID].argumentBufferToUniformBufferMapping.push_back(buffer->Retain());
					_cameraSpecifics[cameraID].vertexShaderUniformBuffers.push_back(metalRenderer->GetUniformBufferReference(totalSize, buffer->GetIndex())->Retain());
				}
			});
			
			const Shader::Signature *fragmentShaderSignature = state->fragmentShader->GetSignature();
			fragmentShaderSignature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				size_t totalSize = buffer->GetTotalUniformSize();
				if(totalSize > 0)
				{
					_cameraSpecifics[cameraID].argumentBufferToUniformBufferMapping.push_back(buffer->Retain());
					_cameraSpecifics[cameraID].fragmentShaderUniformBuffers.push_back(metalRenderer->GetUniformBufferReference(totalSize, buffer->GetIndex())->Retain());
				}
			});
			
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
		Vector4 color;
	};

	struct MetalSpotLight
	{
		Vector3 position;
		float range;
		Vector3 direction;
		float angle;
		Vector4 color;
	};

	struct MetalDirectionalLight
	{
		Vector3 direction;
		float padding;
		Vector4 color;
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

		Camera *camera;
		Vector3 viewPosition;
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;
		
		Color cameraAmbientColor;
		Color cameraFogColor0;
		Color cameraFogColor1;
		Vector2 cameraClipDistance;
		Vector2 cameraFogDistance;
		uint8 multiviewLayer;

		std::vector<uint32> instanceSteps; //number of drawables in the drawables list that use the same pipeline state and can all be rendered with the same draw call as result
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
		const MetalDrawable *currentInstanceDrawable;
	};

	struct MetalWindowInternals
	{
#if RN_PLATFORM_MAC_OS
		NSWindow *window;
#endif
#if RN_PLATFORM_IOS
		RNMetalLayerContainer *metalLayerContainer;
#endif
	};
}

#endif /* __RAYNE_METALINTERNALS_H__ */

//
//  RNVulkanInternals.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANINTERNALS_H__
#define __RAYNE_VULKANINTERNALS_H__

#include "RND3D12Renderer.h"
#include "RND3D12UniformBuffer.h"
#include "RND3D12StateCoordinator.h"

namespace RN
{
	struct D3D12Drawable : public Drawable
	{
		struct CameraSpecific
		{
			const D3D12PipelineState *pipelineState;
			D3D12UniformState *uniformState; //TODO: Check if needs to be deleted when done
		};

		~D3D12Drawable(){}

		void AddUniformStateIfNeeded(size_t cameraID)
		{
			while(_cameraSpecifics.size() <= cameraID)
			{
				_cameraSpecifics.push_back({ nullptr, nullptr });
				dirty = true;
			}
		}

		void UpdateRenderingState(size_t cameraID, const D3D12PipelineState *pipelineState, D3D12UniformState *uniformState)
		{
			_cameraSpecifics[cameraID].pipelineState = pipelineState;
			_cameraSpecifics[cameraID].uniformState = uniformState;
		}

		std::vector<CameraSpecific> _cameraSpecifics;
	};

	struct D3D12LightDirectional
	{
		Vector3 direction;
		float padding;
		Color color;
	};

	struct D3D12RenderPass
	{
		RenderPass *renderPass;
		D3D12Framebuffer *framebuffer;
		Shader::UsageHint shaderHint;
		Material *overrideMaterial;

		Vector3 viewPosition;
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;

		std::vector<D3D12Drawable *> drawables;
		std::vector<D3D12LightDirectional> directionalLights;

		std::vector<Matrix> directionalShadowMatrices;
		D3D12Texture *directionalShadowDepthTexture;
		Vector2 directionalShadowInfo;
	};

	struct D3D12FrameResource
	{
		IUnknown *resource;
		uint32 frame;
	};

	struct D3D12RendererInternals
	{
		std::vector<D3D12RenderPass> renderPasses;
		D3D12StateCoordinator stateCoordinator;

		std::vector<D3D12SwapChain*> swapChains;
		std::vector<D3D12FrameResource> frameResources;

		size_t currentRenderPassIndex;
		size_t currentDrawableResourceIndex;
		size_t totalDrawableCount;

		size_t totalDescriptorTables;
	};

	class D3D12CommandList : public Object
	{
	public:
		friend RN::D3D12Renderer;

		~D3D12CommandList();

		void Begin();
		void End();
		void SetFinishedCallback(std::function<void()> callback);

		ID3D12GraphicsCommandList *GetCommandList() const { return _commandList; }

	protected:
		D3D12CommandList(ID3D12Device *device);

	private:
		void Finish();

		ID3D12GraphicsCommandList *_commandList;
		ID3D12CommandAllocator *_commandAllocator;
		ID3D12Device *_device;
		bool _isOpen;
		UINT _fenceValue;

		std::function<void()> _finishedCallback;

		RNDeclareMetaAPI(D3D12CommandList, D3DAPI)
	};
}

#endif /* __RAYNE_VULKANINTERNALS_H__ */

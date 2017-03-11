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
		~D3D12Drawable()
		{

		}

		void UpdateRenderingState(Renderer *renderer, const D3D12PipelineState *pipelineState, D3D12UniformState *uniformState)
		{
			if(pipelineState == _pipelineState && uniformState == _uniformState)
				return;

			_pipelineState = pipelineState;
			_uniformState = uniformState;
		}

		const D3D12PipelineState *_pipelineState;
		D3D12UniformState *_uniformState;

		D3D12Drawable *_next;
		D3D12Drawable *_prev;

		/*bool _active;
		uint32 _index;*/
	};

	struct D3D12RenderPass
	{
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;

		D3D12Drawable *drawableHead;
		size_t drawableCount;

		size_t textureDescriptorCount;
	};

	struct D3D12RendererInternals
	{
		D3D12RenderPass renderPass;
		D3D12StateCoordinator stateCoordinator;
	};

	class D3D12CommandList : public Object
	{
	public:
		friend RN::D3D12Renderer;

		~D3D12CommandList();

		void Begin();
		void End();

		ID3D12GraphicsCommandList *GetCommandList() const { return _commandList; }

	protected:
		D3D12CommandList(ID3D12Device *device);

	private:
		ID3D12GraphicsCommandList *_commandList;
		ID3D12CommandAllocator *_commandAllocator;
		ID3D12Device *_device;
		bool _isOpen;
		UINT _fenceValue;

		RNDeclareMetaAPI(D3D12CommandList, D3DAPI)
	};

	class D3D12CommandListWithCallback : public D3D12CommandList
	{
	public:
		friend RN::D3D12Renderer;

		~D3D12CommandListWithCallback();
		void SetFinishedCallback(std::function<void()> callback);

	protected:
		D3D12CommandListWithCallback(ID3D12Device *device);

	private:
		std::function<void()> _finishedCallback;

		RNDeclareMetaAPI(D3D12CommandListWithCallback, D3DAPI)
	};
}

#endif /* __RAYNE_VULKANINTERNALS_H__ */

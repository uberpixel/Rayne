//
//  RND3D12Internals.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12INTERNALS_H__
#define __RAYNE_D3D12INTERNALS_H__

#include "d3dx12.h"
#include <Rayne.h>
#include "RND3D12StateCoordinator.h"
#include "RND3D12UniformBuffer.h"

namespace RN
{
	class D3D12Window;
	class Framebuffer;
	class Camera;
	class D3D12GPUBuffer;

	struct D3D12WindowPass
	{
		D3D12Window *window;

/*		id<CAD3D12Drawable> drawable;
		id<MTLTexture> depthTexture;*/
	};

	struct D3D12Drawable : public Drawable
	{
		~D3D12Drawable()
		{
			for(D3D12UniformBuffer *buffer : _vertexBuffers)
				delete buffer;
			for(D3D12UniformBuffer *buffer : _fragmentBuffers)
				delete buffer;
		}

		void UpdateRenderingState(Renderer *renderer, const D3D12RenderingState *state)
		{
			if(state == _pipelineState)
				return;

			_pipelineState = state;

			for(D3D12UniformBuffer *buffer : _vertexBuffers)
				delete buffer;
			for(D3D12UniformBuffer *buffer : _fragmentBuffers)
				delete buffer;

			_vertexBuffers.clear();
			_fragmentBuffers.clear();

			for(D3D12RenderingStateArgument *argument : state->vertexArguments)
			{
				switch(argument->type)
				{
					case D3D12RenderingStateArgument::Type::Buffer:
					{
						if(argument->index > 0)
							_vertexBuffers.push_back(new D3D12UniformBuffer(renderer, static_cast<D3D12RenderingStateUniformBufferArgument *>(argument)));
					}

					default:
						break;
				}
			}

			for(D3D12RenderingStateArgument *argument : state->fragmentArguments)
			{
				switch(argument->type)
				{
					case D3D12RenderingStateArgument::Type::Buffer:
					{
						if(argument->index > 0)
							_fragmentBuffers.push_back(new D3D12UniformBuffer(renderer, static_cast<D3D12RenderingStateUniformBufferArgument *>(argument)));
					}

					default:
						break;
				}
			}
		}

		const D3D12RenderingState *_pipelineState;
		std::vector<D3D12UniformBuffer *> _vertexBuffers;
		std::vector<D3D12UniformBuffer *> _fragmentBuffers;
		D3D12Drawable *_next;
		D3D12Drawable *_prev;
	};

	struct D3D12RenderPass
	{
		Camera *camera;
		Framebuffer *framebuffer;
/*		id<MTLCommandBuffer> commandBuffer;
		id<MTLRenderCommandEncoder> renderCommand;*/
		const D3D12RenderingState *activeState;

		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;

		D3D12Drawable *drawableHead;
		size_t drawableCount;
	};


	class D3D12RendererInternals
	{
	public:
		D3D12WindowPass pass;
		D3D12RenderPass renderPass;
		D3D12StateCoordinator stateCoordinator;

		ID3D12Device *device;
		ID3D12CommandQueue *commandQueue;
		IDXGISwapChain3 *swapChain;
		ID3D12CommandAllocator *commandAllocators[3];
		ID3D12GraphicsCommandList *commandList;

		ID3D12DescriptorHeap *rtvHeap;
		UINT rtvDescriptorSize;
		ID3D12DescriptorHeap *cbvHeap;

		ID3D12Resource *renderTargets[3];
		ID3D12RootSignature *rootSignature;
		
		UINT frameIndex;
		HANDLE fenceEvent;
		ID3D12Fence *fence;
		UINT64 fenceValues[3];

		D3D12_VIEWPORT viewport;
		D3D12_RECT scissorRect;

		void InitializeRenderingPipeline(D3D12Window *window);
		void CreateFramebuffers(const Vector2 &size);
		void GetHardwareAdapter(_In_ IDXGIFactory4* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter);
		void WaitForGpu();
		void MoveToNextFrame();
	};

	LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
}

#endif /* __RAYNE_D3D12INTERNALS_H__ */

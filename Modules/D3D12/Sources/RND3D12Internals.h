//
//  RND3D12Internals.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12INTERNALS_H__
#define __RAYNE_D3D12INTERNALS_H__

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
			bool dirty;
		};

		~D3D12Drawable(){}

		void AddUniformStateIfNeeded(size_t cameraID)
		{
			while(_cameraSpecifics.size() <= cameraID)
			{
				_cameraSpecifics.push_back({ nullptr, nullptr, true });
			}
		}

		void UpdateRenderingState(size_t cameraID, const D3D12PipelineState *pipelineState, D3D12UniformState *uniformState)
		{
			_cameraSpecifics[cameraID].pipelineState = pipelineState;
			_cameraSpecifics[cameraID].uniformState = uniformState;
			_cameraSpecifics[cameraID].dirty = false;
		}

		virtual void MakeDirty() override
		{
			for(int i = 0; i < _cameraSpecifics.size(); i++)
			{
				_cameraSpecifics[i].dirty = true;
			}
		}

		//TODO: This can get somewhat big with lots of post processing stages...
		std::vector<CameraSpecific> _cameraSpecifics;
	};

	struct D3D12LightDirectional
	{
		Vector3 direction;
		float padding;
		Vector4 color;
	};

	struct VulkanPointLight
	{
		Vector3 position;
		float range;
		Vector4 color;
	};

	struct VulkanSpotLight
	{
		Vector3 position;
		float range;
		Vector3 direction;
		float angle;
		Vector4 color;
	};

	struct D3D12RenderPass
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

		Color cameraAmbientColor;
		Color cameraFogColor0;
		Color cameraFogColor1;
		Vector2 cameraClipDistance;
		Vector2 cameraFogDistance;
		uint8 multiviewLayer;

		std::vector<D3D12Drawable *> drawables;
		std::vector<uint32> instanceSteps; //number of drawables in the drawables list that use the same pipeline state and can all be rendered with the same draw call as result

		std::vector<D3D12LightDirectional> directionalLights;
		std::vector<VulkanPointLight> pointLights;
		std::vector<VulkanSpotLight> spotLights;

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

		const D3D12PipelineState *currentPipelineState;
		D3D12Drawable *currentInstanceDrawable;
	};

	class D3D12DescriptorHeap : public Object
	{
	public:
		friend RN::D3D12Renderer;

		~D3D12DescriptorHeap();

		void Reset(size_t size);

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const;
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index) const;

	protected:
		D3D12DescriptorHeap(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);

		D3D12_DESCRIPTOR_HEAP_TYPE _heapType;
		D3D12_DESCRIPTOR_HEAP_FLAGS _heapFlags;
		ID3D12DescriptorHeap *_heap;
		ID3D12Device *_device;

		CD3DX12_CPU_DESCRIPTOR_HANDLE _handleCPU;
		CD3DX12_GPU_DESCRIPTOR_HANDLE _handleGPU;
		size_t _handleIncrement;

		size_t _size;

	private:
		UINT _fenceValue;

		RNDeclareMetaAPI(D3D12DescriptorHeap, D3DAPI)
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

#endif /* __RAYNE_D3D12INTERNALS_H__ */

//
//  RND3D12Renderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Renderer.h"
#include "RND3D12ShaderLibrary.h"
#include "RND3D12GPUBuffer.h"
#include "RND3D12Texture.h"
#include "RND3D12UniformBuffer.h"
#include "RND3D12Device.h"
#include "RND3D12Framebuffer.h"
#include "RND3D12Internals.h"
#include "RND3D12SwapChain.h"
#include "Rendering/RNMesh.h"

namespace RN
{
	RNDefineMeta(D3D12Renderer, Renderer)

	D3D12Renderer::D3D12Renderer(D3D12RendererDescriptor *descriptor, D3D12Device *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_mipMapTextures(new Array()),
		_currentRootSignature(nullptr),
		_submittedCommandLists(new Array()),
		_executedCommandLists(new Array()),
		_commandListPool(new Array()),
		_currentSrvCbvHeap(nullptr),
		_boundDescriptorHeaps(new Array()),
		_descriptorHeapPool(new Array()),
		_scheduledFenceValue(0),
		_completedFenceValue(0),
		_defaultPostProcessingDrawable(nullptr),
		_ppConvertMaterial(nullptr),
		_currentMultiviewLayer(0),
		_currentMultiviewFallbackRenderPass(nullptr)
	{
		ID3D12Device *underlyingDevice = device->GetDevice();

		underlyingDevice->CreateFence(_scheduledFenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

		D3D12_COMMAND_QUEUE_DESC queueDescriptor = {};
		queueDescriptor.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDescriptor.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		underlyingDevice->CreateCommandQueue(&queueDescriptor, IID_PPV_ARGS(&_commandQueue));

		_rtvDescriptorSize = underlyingDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		_defaultShaderLibrary = CreateShaderLibraryWithFile(RNCSTR(":RayneD3D12:/Shaders.json"));
		_uniformBufferPool = new D3D12UniformBufferPool();
	}

	D3D12Renderer::~D3D12Renderer()
	{
		//TODO: Cleanup
		_mainWindow->Release();
		delete _uniformBufferPool;
	}

	D3D12CommandList *D3D12Renderer::GetCommandList()
	{
		D3D12CommandList *commandList = nullptr;
		_lock.Lock();
		if(_commandListPool->GetCount() == 0)
		{
			commandList = new D3D12CommandList(GetD3D12Device()->GetDevice());
		}
		else
		{
			commandList = _commandListPool->GetLastObject<D3D12CommandList>();
			commandList->Retain();
			_commandListPool->RemoveObjectAtIndex(_commandListPool->GetCount() - 1);
			commandList->Begin();
		}
		_lock.Unlock();
		
		return commandList->Autorelease();
	}

	void D3D12Renderer::SubmitCommandList(D3D12CommandList *commandList)
	{
		_lock.Lock();
		_submittedCommandLists->AddObject(commandList);
		_lock.Unlock();
	}

	D3D12DescriptorHeap *D3D12Renderer::GetDescriptorHeap(size_t size, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	{
		D3D12DescriptorHeap *descriptorHeap = nullptr;

		_descriptorHeapPool->Enumerate<D3D12DescriptorHeap>([&](D3D12DescriptorHeap *heap, size_t index, bool &stop) {
			if(heap->_heapType == type && heap->_heapFlags == flags)
			{
				descriptorHeap = heap;
				stop = true;
			}
		});

		if(!descriptorHeap)
		{
			descriptorHeap = new D3D12DescriptorHeap(GetD3D12Device()->GetDevice(), type, flags);
		}
		else
		{
			descriptorHeap->Retain();
			_descriptorHeapPool->RemoveObject(descriptorHeap);
		}

		descriptorHeap->Reset(size);

		return descriptorHeap->Autorelease();
	}

	void D3D12Renderer::SubmitDescriptorHeap(D3D12DescriptorHeap *heap)
	{
		if(heap)
		{
			_boundDescriptorHeaps->AddObject(heap);
			heap->_fenceValue = _scheduledFenceValue;
		}
	}

	Window *D3D12Renderer::CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor)
	{
		D3D12Window *window = new D3D12Window(size, screen, this, descriptor);

		if(!_mainWindow)
			_mainWindow = window->Retain();

		return window;
	}

	void D3D12Renderer::SetMainWindow(Window *window)
	{
		_mainWindow = window;
	}

	Window *D3D12Renderer::GetMainWindow()
	{
		return _mainWindow;
	}


	void D3D12Renderer::CreateMipMapsForTexture(D3D12Texture *texture)
	{
		_lock.Lock();
		_mipMapTextures->AddObject(texture);
		_lock.Unlock();
	}

	void D3D12Renderer::CreateMipMaps()
	{
		if(_mipMapTextures->GetCount() == 0)
			return;

		//Calculate heap size
		uint32 requiredHeapSize = 0;
		bool waitingForUploads = false;
		_mipMapTextures->Enumerate<D3D12Texture>([&](D3D12Texture *texture, size_t index, bool &stop) {
			if(texture->GetDescriptor().mipMaps > 1)
				requiredHeapSize += texture->GetDescriptor().mipMaps - 1;

			if(!texture->_isReady)
			{
				waitingForUploads = true;
				stop = true;
			}
		});

		if(waitingForUploads)
			return;

		//No heap size, means that there was either no texture or none that requires any mipmaps
		if(requiredHeapSize == 0)
		{
			_mipMapTextures->RemoveAllObjects();
			return;
		}

		//Union used for shader constants
		struct DWParam
		{
			DWParam(FLOAT f) : Float(f) {}
			DWParam(UINT u) : Uint(u) {}

			void operator= (FLOAT f) { Float = f; }
			void operator= (UINT u) { Uint = u; }

			union
			{
				FLOAT Float;
				UINT Uint;
			};
		};

		//The compute shader expects 3 floats, the source texture and the destination texture
		CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		rootParameters[0].InitAsConstants(3, 0);
		rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[0]);
		rootParameters[2].InitAsDescriptorTable(1, &srvCbvRanges[1]);

		//Static sampler used to get the linearly interpolated color for the mipmaps
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.MaxAnisotropy = 0;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		samplerDesc.ShaderRegister = 0;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		ID3D12Device *device = GetD3D12Device()->GetDevice();

		//Create the root signature for the mipmap compute shader from the parameters and sampler above
		ID3DBlob *signature;
		ID3DBlob *error;
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		ID3D12RootSignature *mipMapRootSignature;
		device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mipMapRootSignature));

		//Create the descriptor heap with layout: source texture - destination texture
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 2*requiredHeapSize;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ID3D12DescriptorHeap *descriptorHeap;
		device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap));
		UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//Get the compute shader from the default shader library
		D3D12Shader *compute = _defaultShaderLibrary->GetShaderWithName(RNCSTR("GenerateMipMaps"))->Downcast<D3D12Shader>();
		ID3DBlob *computeShader = static_cast<ID3DBlob*>(compute->_shader);

		//Create pipeline state object for the compute shader using the root signature.
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mipMapRootSignature;
		psoDesc.CS = { reinterpret_cast<UINT8*>(computeShader->GetBufferPointer()), computeShader->GetBufferSize() };
		ID3D12PipelineState *psoMipMaps;
		device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&psoMipMaps));

		//Prepare the shader resource view description for the source texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srcTextureSRVDesc = {};
		srcTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srcTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		//Prepare the unordered access view description for the destination texture
		D3D12_UNORDERED_ACCESS_VIEW_DESC destTextureUAVDesc = {};
		destTextureUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		//Get a new empty command list in recording state
		D3D12CommandList *rnCommandList = GetCommandList();
		ID3D12GraphicsCommandList *commandList = rnCommandList->GetCommandList();

		//Set root signature, pso and descriptor heap
		commandList->SetComputeRootSignature(mipMapRootSignature);
		commandList->SetPipelineState(psoMipMaps);
		commandList->SetDescriptorHeaps(1, &descriptorHeap);

		//CPU handle for the first descriptor on the descriptor heap, used to fill the heap
		CD3DX12_CPU_DESCRIPTOR_HANDLE currentCPUHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, descriptorSize);

		//GPU handle for the first descriptor on the descriptor heap, used to initialize the descriptor tables
		CD3DX12_GPU_DESCRIPTOR_HANDLE currentGPUHandle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, descriptorSize);

		std::vector<IUnknown*> temporaryResources;

		_mipMapTextures->Enumerate<D3D12Texture>([&](D3D12Texture *texture, size_t index, bool &stop) {
			const Texture::Descriptor &textureDescriptor = texture->GetDescriptor();

			//Skip textures without mipmaps
			if(textureDescriptor.mipMaps <= 1 || !texture->_resource)
				return;

			//TODO: Do this only for textures not supporting unordered access
			//Create temporary texture resource supporting unordered access (SRGB textures do not!)
			D3D12_RESOURCE_DESC tempTextureDescriptor = {};
			tempTextureDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//TODO: Support other texture dimensions
			tempTextureDescriptor.Alignment = 0;
			tempTextureDescriptor.Width = textureDescriptor.width;
			tempTextureDescriptor.Height = textureDescriptor.height;
			tempTextureDescriptor.DepthOrArraySize = textureDescriptor.depth;
			tempTextureDescriptor.MipLevels = textureDescriptor.mipMaps;
			tempTextureDescriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//TODO: Base the format on the original texture
			tempTextureDescriptor.SampleDesc.Count = 1;
			tempTextureDescriptor.SampleDesc.Quality = 0;
			tempTextureDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			tempTextureDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			D3D12MA::Allocation *textureAllocation;
			
			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			GetD3D12Device()->GetMemoryAllocator()->CreateResource(&allocationDesc, &tempTextureDescriptor, D3D12_RESOURCE_STATE_COPY_DEST, NULL, &textureAllocation, IID_NULL, NULL);
			temporaryResources.push_back(textureAllocation);
			ID3D12Resource* textureResource = textureAllocation->GetResource();

			//Transition from pixel shader resource to copy source
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->_resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE));

			//Copy into the temporary resource
			commandList->CopyResource(textureResource, texture->_resource);

			//Transition temporary resource from copy dest to unordered access
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

			D3D12_RESOURCE_BARRIER uavToSrvBarrier = {};
			uavToSrvBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			uavToSrvBarrier.Transition.pResource = textureResource;
			uavToSrvBarrier.Transition.Subresource = 0;
			uavToSrvBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			uavToSrvBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

			commandList->ResourceBarrier(1, &uavToSrvBarrier);

			//Loop through the mipmaps copying from the bigger mipmap to the smaller one with downsampling in a compute shader
			for(uint32_t TopMip = 0; TopMip < textureDescriptor.mipMaps-1; TopMip++)
			{
				//Get mipmap dimensions
				uint32_t dstWidth = std::max(textureDescriptor.width >> (TopMip+1), 1u);
				uint32_t dstHeight = std::max(textureDescriptor.height >> (TopMip+1), 1u);

				//Create shader resource view for the source texture in the descriptor heap
				srcTextureSRVDesc.Format = tempTextureDescriptor.Format;
				srcTextureSRVDesc.Texture2D.MipLevels = 1;
				srcTextureSRVDesc.Texture2D.MostDetailedMip = TopMip;
				device->CreateShaderResourceView(textureResource, &srcTextureSRVDesc, currentCPUHandle);
				currentCPUHandle.Offset(1, descriptorSize);

				//Create unordered access view for the destination texture in the descriptor heap
				destTextureUAVDesc.Format = tempTextureDescriptor.Format;
				destTextureUAVDesc.Texture2D.MipSlice = TopMip+1;
				device->CreateUnorderedAccessView(textureResource, nullptr, &destTextureUAVDesc, currentCPUHandle);
				currentCPUHandle.Offset(1, descriptorSize);

				//Pass the destination texture pixel size to the shader as constants
				commandList->SetComputeRoot32BitConstant(0, DWParam(1.0f/dstWidth).Uint, 0);
				commandList->SetComputeRoot32BitConstant(0, DWParam(1.0f/dstHeight).Uint, 1);
				commandList->SetComputeRoot32BitConstant(0, DWParam((textureDescriptor.format == Texture::Format::RGBA_8_SRGB)?2.2f:1.0f).Uint, 2);	//TODO: Adjust gamma curve based on the original texture format

				//Pass the source and destination texture views to the shader via descriptor tables
				commandList->SetComputeRootDescriptorTable(1, currentGPUHandle);
				currentGPUHandle.Offset(1, descriptorSize);
				commandList->SetComputeRootDescriptorTable(2, currentGPUHandle);
				currentGPUHandle.Offset(1, descriptorSize);

				//Dispatch the compute shader with one thread per 8x8 pixels
				commandList->Dispatch(std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), 1);

				//Wait for all accesses to the destination texture UAV to be finished before generating the next mipmap, as it will be the source texture for the next mipmap
				commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(textureResource));

				uavToSrvBarrier.Transition.Subresource = TopMip + 1;
				commandList->ResourceBarrier(1, &uavToSrvBarrier);
			}

			//Copy temp resource back into the original one
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->_resource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE));
			commandList->CopyResource(texture->_resource, textureResource);

			//When done with the texture, transition it's state back to be a pixel shader resource
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->_resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
			texture->_needsMipMaps = false;
		});

		//Close and submit the command list
		rnCommandList->End();
		rnCommandList->SetFinishedCallback([temporaryResources, descriptorHeap, mipMapRootSignature, psoMipMaps](){
			//Cleanup
			for(IUnknown *resource : temporaryResources)
			{
				resource->Release();
			}

			descriptorHeap->Release();
			mipMapRootSignature->Release();
			psoMipMaps->Release();
		});
		SubmitCommandList(rnCommandList);

		_mipMapTextures->RemoveAllObjects();
	}


	void D3D12Renderer::Render(Function &&function)
	{
		_currentDrawableIndex = 0;
		_internals->renderPasses.clear();
		_internals->totalDrawableCount = 0;
		_internals->currentRenderPassIndex = 0;
		_internals->currentDrawableResourceIndex = 0;
		_internals->totalDescriptorTables = 0;
		_internals->swapChains.clear();
		_currentRootSignature = nullptr;
		_currentSrvCbvIndex = 0;

		_completedFenceValue = _fence->GetCompletedValue();

		//Delete command lists that finished execution on the graphics card (the command allocator needs to be alive the whole time)
		for(int i = _executedCommandLists->GetCount() - 1; i >= 0; i--)
		{
			if(_executedCommandLists->GetObjectAtIndex<D3D12CommandList>(i)->_fenceValue <= _completedFenceValue)
			{
				D3D12CommandList *commandList = _executedCommandLists->GetObjectAtIndex<D3D12CommandList>(i);
				commandList->Finish();
				_commandListPool->AddObject(commandList);
				_executedCommandLists->RemoveObjectAtIndex(i);
			}
		}

		//Add descriptor heaps that aren't in use by the GPU anymore back to the pool
		for(int i = _boundDescriptorHeaps->GetCount() - 1; i >= 0; i--)
		{
			if(_boundDescriptorHeaps->GetObjectAtIndex<D3D12DescriptorHeap>(i)->_fenceValue <= _completedFenceValue)
			{
				D3D12DescriptorHeap *descriptorHeap = _boundDescriptorHeaps->GetObjectAtIndex<D3D12DescriptorHeap>(i);
				_descriptorHeapPool->AddObject(descriptorHeap);
				_boundDescriptorHeaps->RemoveObjectAtIndex(i);
			}
		}

		//Free other frame resources such as descriptor heaps, that are not in use by the gpu anymore
		for(int i = _internals->frameResources.size()-1; i >= 0; i--)
		{
			D3D12FrameResource &frameResource = _internals->frameResources[i];
			if(frameResource.frame <= _completedFenceValue)
			{
				frameResource.resource->Release();
				_internals->frameResources.erase(_internals->frameResources.begin() + i);
			}
		}

		CreateMipMaps();

		//SubmitCamera is called for each camera and creates lists of drawables per camera
		function();

		_internals->currentPipelineState = nullptr; //This is used when submitting drawables to make lists of drawables to instance and needs to be reset per render pass
		_internals->currentInstanceDrawable = nullptr;

		for(D3D12SwapChain *swapChain : _internals->swapChains)
		{
			swapChain->AcquireBackBuffer();
		}

		_uniformBufferPool->Update(_scheduledFenceValue, _completedFenceValue);
		PopulateDescriptorHeap();
		SubmitDescriptorHeap(_currentSrvCbvHeap);

		if(_internals->swapChains.size() > 0)
		{
			_currentCommandList = GetCommandList();

			for(D3D12SwapChain *swapChain : _internals->swapChains)
			{
				swapChain->Prepare(_currentCommandList);
			}

			ID3D12GraphicsCommandList *commandList = _currentCommandList->GetCommandList();

			_internals->currentRenderPassIndex = 0;
			_internals->currentDrawableResourceIndex = 0;
			for(const D3D12RenderPass &renderPass : _internals->renderPasses)
			{
				if(renderPass.type != D3D12RenderPass::Type::Default && renderPass.type != D3D12RenderPass::Type::Convert)
				{
					RenderAPIRenderPass(_currentCommandList, renderPass);
					_internals->currentRenderPassIndex += 1;
					continue;
				}
				SetupRendertargets(_currentCommandList, renderPass);
				
				if(renderPass.drawables.size() > 0)
				{
					//Draw drawables
					uint32 stepSize = 0;
					uint32 stepSizeIndex = 0;
					for(size_t i = 0; i < renderPass.drawables.size(); i += stepSize)
					{
						//TODO: Sort drawables by camera and root signature
						if(renderPass.drawables[i]->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature != _currentRootSignature)
						{
							_currentRootSignature = renderPass.drawables[i]->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature;
							commandList->SetGraphicsRootSignature(_currentRootSignature->signature);

							// Set the one big descriptor heap for the whole frame
							ID3D12DescriptorHeap* srvCbvHeaps[] = { _currentSrvCbvHeap->_heap };
							commandList->SetDescriptorHeaps(_countof(srvCbvHeaps), srvCbvHeaps);
						}

						stepSize = renderPass.instanceSteps[stepSizeIndex++];
						RenderDrawable(commandList, renderPass.drawables[i], stepSize);
					}

					_internals->currentDrawableResourceIndex += 1;
				}

				_internals->currentRenderPassIndex += 1;
			}

			for(D3D12SwapChain *swapChain : _internals->swapChains)
			{
				swapChain->Finalize(_currentCommandList);
			}

			_uniformBufferPool->FlushAllBuffers();

			_currentCommandList->End();
			SubmitCommandList(_currentCommandList);
			_currentCommandList = nullptr;
		}

		// Execute all command lists
		std::vector<ID3D12CommandList*> commandLists;
		_lock.Lock();
		_submittedCommandLists->Enumerate<D3D12CommandList>([&](D3D12CommandList *list, size_t index, bool &stop) {
			commandLists.push_back(list->GetCommandList());
			list->_fenceValue = _scheduledFenceValue;
		});
		_executedCommandLists->AddObjectsFromArray(_submittedCommandLists);
		_submittedCommandLists->RemoveAllObjects();
		_lock.Unlock();
		if(commandLists.size() > 0)
			_commandQueue->ExecuteCommandLists(commandLists.size(), &commandLists[0]);

		_commandQueue->Signal(_fence, _scheduledFenceValue++);

		for(D3D12SwapChain *swapChain : _internals->swapChains)
		{
			swapChain->PresentBackBuffer();
		}
	}

	//TODO: Merge parts of this with SubmitRenderPass and call it in here
	void D3D12Renderer::SubmitCamera(Camera *camera, Function &&function)
	{
		const Array *multiviewCameras = camera->GetMultiviewCameras();
		if (multiviewCameras && multiviewCameras->GetCount() > 0)
		{
			//TODO: Multiview is not supported by metal, should use instanced with viewport selection instead (https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/rendering_to_multiple_texture_slices_in_a_draw_command?language=objc)
			/*if(multiviewCameras->GetCount() > 1 && GetVulkanDevice()->GetSupportsMultiview())
			{

			}
			else*/
			{
				//If multiview is not supported or there is only one multiview camera, render them individually into the correct framebuffer texture layers
				multiviewCameras->Enumerate<Camera>([&](Camera *multiviewCamera, size_t index, bool &stop) {
					_currentMultiviewLayer = index;
					_currentMultiviewFallbackRenderPass = camera->GetRenderPass();

					SubmitCamera(multiviewCamera, std::move(function));

					_currentMultiviewLayer = 0;
					_currentMultiviewFallbackRenderPass = nullptr;
				});

				return;
			}
		}

		RenderPass *cameraRenderPass = _currentMultiviewFallbackRenderPass ? _currentMultiviewFallbackRenderPass : camera->GetRenderPass();

		_internals->currentPipelineState = nullptr; //This is used when submitting drawables to make lists of drawables to instance and needs to be reset per render pass
		_internals->currentInstanceDrawable = nullptr;
		
		D3D12RenderPass renderPass;
		renderPass.drawables.resize(0);
		renderPass.multiviewLayer = _currentMultiviewLayer;

		renderPass.type = D3D12RenderPass::Type::Default;
		renderPass.renderPass = cameraRenderPass;
		renderPass.previousRenderPass = nullptr;

		renderPass.shaderHint = camera->GetShaderHint();
		renderPass.overrideMaterial = camera->GetMaterial();

		renderPass.viewPosition = camera->GetWorldPosition();
		renderPass.viewMatrix = camera->GetViewMatrix();
		renderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		renderPass.projectionMatrix = camera->GetProjectionMatrix();
		//renderPass.projectionMatrix.m[5] *= -1.0f;
		renderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		renderPass.projectionViewMatrix = renderPass.projectionMatrix * renderPass.viewMatrix;
		renderPass.directionalShadowDepthTexture = nullptr;

		renderPass.cameraAmbientColor = camera->GetAmbientColor();
		renderPass.cameraFogColor0 = camera->GetFogColor0();
		renderPass.cameraFogColor1 = camera->GetFogColor1();
		renderPass.cameraClipDistance = RN::Vector2(camera->GetClipNear(), camera->GetClipFar());
		renderPass.cameraFogDistance = RN::Vector2(camera->GetFogNear(), camera->GetFogFar());

		Framebuffer *framebuffer = cameraRenderPass->GetFramebuffer();
		D3D12SwapChain *newSwapChain = nullptr;
		newSwapChain = framebuffer->Downcast<D3D12Framebuffer>()->GetSwapChain();
		renderPass.framebuffer = framebuffer->Downcast<D3D12Framebuffer>();
		
		if(newSwapChain)
		{
			bool notIncluded = true;
			for(D3D12SwapChain *swapChain : _internals->swapChains)
			{
				if(swapChain == newSwapChain)
				{
					notIncluded = false;
					break;
				}
			}

			if(notIncluded)
			{
				_internals->swapChains.push_back(newSwapChain);
			}
		}

		_internals->currentRenderPassIndex = _internals->renderPasses.size();
		_internals->renderPasses.push_back(renderPass);

		// Create drawables
		function();

		size_t numberOfDrawables = _internals->renderPasses[_internals->currentRenderPassIndex].drawables.size();
		_internals->totalDrawableCount += numberOfDrawables;

		if(numberOfDrawables > 0)
			_internals->currentDrawableResourceIndex += 1;


		const Array *nextRenderPasses = cameraRenderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop)
		{
			SubmitRenderPass(nextPass, renderPass.renderPass);
		});
	}

	void D3D12Renderer::SubmitRenderPass(RenderPass *renderPass, RenderPass *previousRenderPass)
	{
		D3D12RenderPass d3dRenderPass;
		d3dRenderPass.drawables.resize(0);
		d3dRenderPass.multiviewLayer = 0;

		_internals->currentPipelineState = nullptr; //This is used when submitting drawables to make lists of drawables to instance and needs to be reset per render pass
		_internals->currentInstanceDrawable = nullptr;

		PostProcessingAPIStage *apiStage = renderPass->Downcast<PostProcessingAPIStage>();
		PostProcessingStage *ppStage = renderPass->Downcast<PostProcessingStage>();

		d3dRenderPass.renderPass = renderPass;
		d3dRenderPass.previousRenderPass = previousRenderPass;

		d3dRenderPass.shaderHint = Shader::UsageHint::Default;
		d3dRenderPass.overrideMaterial = ppStage ? ppStage->GetMaterial() : nullptr;

		d3dRenderPass.directionalShadowDepthTexture = nullptr;

		if(!apiStage)
		{
			d3dRenderPass.type = D3D12RenderPass::Type::Default;
		}
		else
		{
			switch(apiStage->GetType())
			{
				case PostProcessingAPIStage::Type::ResolveMSAA:
				{
					d3dRenderPass.type = D3D12RenderPass::Type::ResolveMSAA;
					break;
				}
				case PostProcessingAPIStage::Type::Blit:
				{
					d3dRenderPass.type = D3D12RenderPass::Type::Blit;
					break;
				}
				case PostProcessingAPIStage::Type::Convert:
				{
					d3dRenderPass.type = D3D12RenderPass::Type::Convert;

					if(!_ppConvertMaterial)
					{
						_ppConvertMaterial = Material::WithShaders(_defaultShaderLibrary->GetShaderWithName(RNCSTR("pp_vertex")), _defaultShaderLibrary->GetShaderWithName(RNCSTR("pp_blit_fragment")));
					}
					d3dRenderPass.overrideMaterial = _ppConvertMaterial;
					break;
				}
			}
		}

		Framebuffer *framebuffer = renderPass->GetFramebuffer();
		D3D12SwapChain *newSwapChain = nullptr;
		newSwapChain = framebuffer->Downcast<D3D12Framebuffer>()->GetSwapChain();
		d3dRenderPass.framebuffer = framebuffer->Downcast<D3D12Framebuffer>();

		if(newSwapChain)
		{
			bool notIncluded = true;
			for (D3D12SwapChain *swapChain : _internals->swapChains)
			{
				if (swapChain == newSwapChain)
				{
					notIncluded = false;
					break;
				}
			}

			if (notIncluded)
			{
				_internals->swapChains.push_back(newSwapChain);
			}
		}

		_internals->currentRenderPassIndex = _internals->renderPasses.size();
		_internals->renderPasses.push_back(d3dRenderPass);

		if(ppStage || d3dRenderPass.type == D3D12RenderPass::Type::Convert)
		{
			//Submit fullscreen quad drawable
			if(!_defaultPostProcessingDrawable)
			{
				Mesh *planeMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 90.0f, 0.0f)), Vector3(0.0f), Vector2(1.0f, 1.0f));
				Material *planeMaterial = Material::WithShaders(GetDefaultShader(Shader::Type::Vertex, nullptr), GetDefaultShader(Shader::Type::Fragment, nullptr));

				_lock.Lock();
				_defaultPostProcessingDrawable = static_cast<D3D12Drawable*>(CreateDrawable());
				_defaultPostProcessingDrawable->mesh = planeMesh->Retain();
				_defaultPostProcessingDrawable->material = planeMaterial->Retain();
				_lock.Unlock();
			}

/*			Texture *sourceTexture = d3dRenderPass.previousRenderPass->GetFramebuffer()->GetColorTexture(0);
			if(d3dRenderPass.type == D3D12RenderPass::Type::Convert)
			{
				_defaultPostProcessingDrawable->material->RemoveAllTextures();
				_defaultPostProcessingDrawable->material->AddTexture(sourceTexture);
			}*/
			SubmitDrawable(_defaultPostProcessingDrawable);

			size_t numberOfDrawables = _internals->renderPasses[_internals->currentRenderPassIndex].drawables.size();
			_internals->totalDrawableCount += numberOfDrawables;

			if (numberOfDrawables > 0)
				_internals->currentDrawableResourceIndex += 1;
		}

		const Array *nextRenderPasses = renderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop)
		{
			SubmitRenderPass(nextPass, renderPass);
		});
	}

	GPUBuffer *D3D12Renderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions, bool isStreamable)
	{
		return new D3D12GPUBuffer(nullptr, length, usageOptions);
	}
	GPUBuffer *D3D12Renderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions, bool isStreamable)
	{
		return new D3D12GPUBuffer(bytes, length, usageOptions);
	}

	D3D12UniformBufferReference *D3D12Renderer::GetUniformBufferReference(size_t size, size_t index)
	{
		D3D12UniformBufferReference *reference = _uniformBufferPool->GetUniformBufferReference(size, index);
		return reference;
	}

	void D3D12Renderer::UpdateUniformBufferReference(D3D12UniformBufferReference *reference, bool align)
	{
		LockGuard<Lockable> lock(_lock);
		return _uniformBufferPool->UpdateUniformBufferReference(reference, align);
	}

	ShaderLibrary *D3D12Renderer::CreateShaderLibraryWithFile(const String *file)
	{
		return new D3D12ShaderLibrary(file);
	}

	ShaderLibrary *D3D12Renderer::CreateShaderLibraryWithSource(const String *source)
	{
		D3D12ShaderLibrary *lib = new D3D12ShaderLibrary(nullptr);
		return lib;
	}

	ShaderLibrary *D3D12Renderer::GetDefaultShaderLibrary()
	{
		return _defaultShaderLibrary;
	}

	bool D3D12Renderer::SupportsTextureFormat(const String *format) const
	{
		//TODO: Fix this
		return true;
	}

	bool D3D12Renderer::SupportsDrawMode(DrawMode mode) const
	{
		return true;
	}

	size_t D3D12Renderer::GetAlignmentForType(PrimitiveType type) const
	{
		// TODO!!!!
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
			case PrimitiveType::Half:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
			case PrimitiveType::HalfVector2:
				return 4;

			case PrimitiveType::Vector2:
			case PrimitiveType::HalfVector3:
			case PrimitiveType::HalfVector4:
			case PrimitiveType::Matrix2x2:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Matrix3x3:
			case PrimitiveType::Matrix4x4:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;

			default:
				return 1;
		}
	}

	size_t D3D12Renderer::GetSizeForType(PrimitiveType type) const
	{
		// TODO!!!
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
			case PrimitiveType::Half:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
			case PrimitiveType::HalfVector2:
				return 4;

			case PrimitiveType::Vector2:
			case PrimitiveType::HalfVector3:
			case PrimitiveType::HalfVector4:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Matrix2x2:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;

			case PrimitiveType::Matrix3x3:
				return 48;

			case PrimitiveType::Matrix4x4:
				return 64;

			default:
				return 1;
		}
	}

	Texture *D3D12Renderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		return new D3D12Texture(descriptor, this);
	}

	Framebuffer *D3D12Renderer::CreateFramebuffer(const Vector2 &size)
	{
		return new D3D12Framebuffer(size, this);
	}

	Drawable *D3D12Renderer::CreateDrawable()
	{
		D3D12Drawable *newDrawable = new D3D12Drawable();
		return newDrawable;
	}

	void D3D12Renderer::DeleteDrawable(Drawable *drawable)
	{
		delete drawable;
	}

	void D3D12Renderer::SetupRendertargets(D3D12CommandList *commandList, const D3D12RenderPass &renderpass)
	{
		ID3D12Device *underlyingDevice = GetD3D12Device()->GetDevice();
		ID3D12GraphicsCommandList *d3dCommandList = commandList->GetCommandList();

		//TODO: Call PrepareAsRendertargetForFrame() only once per framebuffer per frame
		renderpass.framebuffer->PrepareAsRendertargetForFrame(_scheduledFenceValue, renderpass.multiviewLayer, 1);
		renderpass.framebuffer->SetAsRendertarget(commandList);

		//Setup viewport and scissor rect
		Rect cameraRect = renderpass.renderPass->GetFrame();

		D3D12_VIEWPORT viewport;
		viewport.Width = cameraRect.width;
		viewport.Height = cameraRect.height;
		viewport.TopLeftX = cameraRect.x;
		viewport.TopLeftY = cameraRect.y;
		viewport.MaxDepth = 1;
		viewport.MinDepth = 0;

		D3D12_RECT viewportRect;
		viewportRect.right = static_cast<LONG>(cameraRect.width + cameraRect.x);
		viewportRect.bottom = static_cast<LONG>(cameraRect.height + cameraRect.y);
		viewportRect.left = cameraRect.x;
		viewportRect.top = cameraRect.y;

		d3dCommandList->RSSetViewports(1, &viewport);
		d3dCommandList->RSSetScissorRects(1, &viewportRect);

		
		// Cameras always clear the whole framebuffer to be more consistent with the metal renderer
		if(renderpass.renderPass->GetFlags() & RenderPass::Flags::ClearColor)
		{
			renderpass.framebuffer->ClearColorTargets(commandList, renderpass.renderPass->GetClearColor());
		}
		
		if(renderpass.renderPass->GetFlags() & RenderPass::Flags::ClearDepthStencil)
		{
			renderpass.framebuffer->ClearDepthStencilTarget(commandList, renderpass.renderPass->GetClearDepth(), renderpass.renderPass->GetClearStencil());
		}
	}

	void D3D12Renderer::PopulateDescriptorHeap()
	{
		_internals->currentRenderPassIndex = 0;
		_internals->currentDrawableResourceIndex = 0;

		if(_internals->totalDrawableCount == 0)
		{
			_currentSrvCbvHeap = nullptr;
			return;
		}
			
		ID3D12Device *device = GetD3D12Device()->GetDevice();

		_currentSrvCbvHeap = GetDescriptorHeap(_internals->totalDescriptorTables, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

		// Describe null SRVs. Null descriptors are used to "unbind" textures
		D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		nullSrvDesc.Texture2D.MipLevels = 1;
		nullSrvDesc.Texture2D.MostDetailedMip = 0;
		nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		size_t heapIndex = 0;
		CD3DX12_CPU_DESCRIPTOR_HANDLE currentCPUHandle = _currentSrvCbvHeap->GetCPUHandle(heapIndex);

		for(const D3D12RenderPass &renderPass : _internals->renderPasses)
		{
			if(renderPass.drawables.size() == 0)
			{
				_internals->currentRenderPassIndex += 1;
				continue;
			}

			size_t stepSize = 0;
			uint32 stepSizeIndex = 0;
			for(size_t i = 0; i < renderPass.drawables.size(); i += stepSize)
			{
				stepSize = renderPass.instanceSteps[stepSizeIndex++];

				D3D12Drawable *drawable = renderPass.drawables[i];

				//The order of descriptors here needs to match the order in the root signature for each table
				//
				//Create constant buffer descriptors
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				D3D12UniformState *uniformState = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState;
				size_t counter = 0;
				for(size_t bufferIndex = 0; bufferIndex < uniformState->vertexUniformBuffers.size(); bufferIndex += 1)
				{
					Shader::ArgumentBuffer *argument = uniformState->uniformBufferToArgumentMapping[counter++];

					//Setup uniforms for all instances that are part of this draw call
					for(size_t instance = 0; instance < stepSize; instance += 1)
					{
						if(instance > 0 && argument->GetMaxInstanceCount() == 1) break;

						D3D12UniformState *instanceUniformState = renderPass.drawables[i + instance]->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState;
						UpdateUniformBufferReference(instanceUniformState->vertexUniformBuffers[bufferIndex], instance == 0);
						FillUniformBuffer(argument, instanceUniformState->vertexUniformBuffers[bufferIndex], renderPass.drawables[i + instance]);
					}

					D3D12UniformBufferReference *uniformBuffer = uniformState->vertexUniformBuffers[bufferIndex];
					D3D12GPUBuffer *actualBuffer = uniformBuffer->uniformBuffer->GetActiveBuffer()->Downcast<D3D12GPUBuffer>();
					cbvDesc.BufferLocation = actualBuffer->GetD3D12Resource()->GetGPUVirtualAddress() + uniformBuffer->offset;
					cbvDesc.SizeInBytes = uniformBuffer->size *std::min(stepSize, argument->GetMaxInstanceCount());
					cbvDesc.SizeInBytes = cbvDesc.SizeInBytes + kRNUniformBufferAlignement - (cbvDesc.SizeInBytes % kRNUniformBufferAlignement); //Does this really need to be aligned here!?
					GetD3D12Device()->GetDevice()->CreateConstantBufferView(&cbvDesc, currentCPUHandle);
					currentCPUHandle = _currentSrvCbvHeap->GetCPUHandle(++heapIndex);
				}

				//TODO: Add support for vertex shader textures here

				for(size_t bufferIndex = 0; bufferIndex < uniformState->fragmentUniformBuffers.size(); bufferIndex += 1)
				{
					Shader::ArgumentBuffer *argument = uniformState->uniformBufferToArgumentMapping[counter++];
					
					//Setup uniforms for all instances that are part of this draw call
					for(size_t instance = 0; instance < stepSize; instance += 1)
					{
						if(instance > 0 && argument->GetMaxInstanceCount() == 1) break;

						D3D12UniformState *instanceUniformState = renderPass.drawables[i + instance]->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState;
						UpdateUniformBufferReference(instanceUniformState->fragmentUniformBuffers[bufferIndex], instance == 0);
						FillUniformBuffer(argument, instanceUniformState->fragmentUniformBuffers[bufferIndex], renderPass.drawables[i + instance]);
					}

					D3D12UniformBufferReference *uniformBuffer = uniformState->fragmentUniformBuffers[bufferIndex];
					D3D12GPUBuffer *actualBuffer = uniformBuffer->uniformBuffer->GetActiveBuffer()->Downcast<D3D12GPUBuffer>();
					cbvDesc.BufferLocation = actualBuffer->GetD3D12Resource()->GetGPUVirtualAddress() + uniformBuffer->offset;
					cbvDesc.SizeInBytes = uniformBuffer->size *std::min(stepSize, argument->GetMaxInstanceCount());
					cbvDesc.SizeInBytes  = cbvDesc.SizeInBytes + kRNUniformBufferAlignement - (cbvDesc.SizeInBytes % kRNUniformBufferAlignement); //Does this really need to be aligned here!?
					GetD3D12Device()->GetDevice()->CreateConstantBufferView(&cbvDesc, currentCPUHandle);
					currentCPUHandle = _currentSrvCbvHeap->GetCPUHandle(++heapIndex);
				}
				
				Shader *fragmentShader = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->descriptor.fragmentShader;
				if(fragmentShader)
				{
					//Create texture descriptors
					const Array *textures = drawable->material->GetTextures();
					fragmentShader->GetSignature()->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t i, bool &stop) {
						ID3D12Resource *textureResource = nullptr;
						D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor = nullSrvDesc;
						if(argument->GetMaterialTextureIndex() == Shader::ArgumentTexture::IndexDirectionalShadowTexture)
						{
							const D3D12Texture *materialTexture = renderPass.directionalShadowDepthTexture;
							if(materialTexture && materialTexture->_isReady && !materialTexture->_needsMipMaps)
							{
								textureResource = materialTexture->_resource;
								srvDescriptor = materialTexture->_srvDescriptor;
							}
						}
						else if(argument->GetMaterialTextureIndex() == Shader::ArgumentTexture::IndexFramebufferTexture && renderPass.previousRenderPass && renderPass.previousRenderPass->GetFramebuffer())
						{
							D3D12Framebuffer *framebuffer = renderPass.previousRenderPass->GetFramebuffer()->Downcast<D3D12Framebuffer>();
							Texture *texture = framebuffer->GetColorTexture();
							D3D12Texture *d3dTexture = nullptr;
							if(texture)
							{
								d3dTexture = texture->Downcast<D3D12Texture>();
									textureResource = d3dTexture->_resource;
								srvDescriptor = d3dTexture->_srvDescriptor;
							}
							else
							{
								textureResource = framebuffer->GetSwapChainColorBuffer();

								srvDescriptor.Format = framebuffer->_colorTargets[0]->d3dTargetViewDesc.Format;
								srvDescriptor.ViewDimension = static_cast<D3D12_SRV_DIMENSION>(framebuffer->_colorTargets[0]->d3dTargetViewDesc.ViewDimension);
								srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

								//TODO: Move this in it's own mapping function supporting all possible cases
								if(srvDescriptor.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2D)
								{
									srvDescriptor.Texture2D.MipLevels = 1;
									srvDescriptor.Texture2D.MostDetailedMip = 0;
									srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
									srvDescriptor.Texture2D.PlaneSlice = 0;
								}
								else if(srvDescriptor.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
								{
									srvDescriptor.Texture2DArray.MipLevels = 1;
									srvDescriptor.Texture2DArray.MostDetailedMip = 0;
									srvDescriptor.Texture2DArray.ResourceMinLODClamp = 0.0f;
									srvDescriptor.Texture2DArray.PlaneSlice = 0;
									srvDescriptor.Texture2DArray.FirstArraySlice = 0;
									srvDescriptor.Texture2DArray.ArraySize = framebuffer->_colorTargets[0]->d3dTargetViewDesc.Texture2DArray.ArraySize;
								}
							}

							if(textureResource)
							{
								//Check if texture finished uploading to the vram
								if(d3dTexture && (d3dTexture->_isReady || d3dTexture->_needsMipMaps))
								{
									textureResource = nullptr;
								}
							}
						}
						else if(argument->GetMaterialTextureIndex() < textures->GetCount())
						{
							const D3D12Texture *materialTexture = textures->GetObjectAtIndex<D3D12Texture>(argument->GetMaterialTextureIndex());
							if(materialTexture && materialTexture->_isReady && !materialTexture->_needsMipMaps)
							{
								textureResource = materialTexture->_resource;
								srvDescriptor = materialTexture->_srvDescriptor;
							}
						}

						device->CreateShaderResourceView(textureResource, &srvDescriptor, currentCPUHandle);
						currentCPUHandle = _currentSrvCbvHeap->GetCPUHandle(++heapIndex);
					});
				}
			}

			_internals->currentRenderPassIndex += 1;
			_internals->currentDrawableResourceIndex += 1;
		}
	}

	void D3D12Renderer::FillUniformBuffer(Shader::ArgumentBuffer *argument, D3D12UniformBufferReference *uniformBufferReference, D3D12Drawable *drawable)
	{
		GPUBuffer *gpuBuffer = uniformBufferReference->uniformBuffer->GetActiveBuffer();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer()) + uniformBufferReference->offset;

		Material *overrideMaterial = _internals->renderPasses[_internals->currentRenderPassIndex].overrideMaterial;
		const Material::Properties &mergedMaterialProperties = drawable->material->GetMergedProperties(overrideMaterial);
		const D3D12RenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];

		argument->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {
			
			switch(descriptor->GetIdentifier())
			{
				case Shader::UniformDescriptor::Identifier::Time:
				{
					float temp = static_cast<float>(Kernel::GetSharedInstance()->GetTotalTime());
					std::memcpy(buffer + descriptor->GetOffset(), &temp, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), drawable->modelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::NormalMatrix:
				{
					Matrix normalMatrix = drawable->inverseModelMatrix.GetTransposed();
					std::memcpy(buffer + descriptor->GetOffset(), normalMatrix.m, descriptor->GetSize());
					break;
				}
				
				case Shader::UniformDescriptor::Identifier::ModelViewMatrix:
				{
					Matrix result = renderPass.viewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewProjectionMatrix:
				{
					Matrix result = renderPass.projectionViewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.viewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.projectionViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.projectionMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), drawable->inverseModelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewMatrix:
				{
					Matrix result = renderPass.inverseViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewProjectionMatrix:
				{
					Matrix result = renderPass.inverseProjectionViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.inverseViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.inverseProjectionViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.inverseProjectionMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::AmbientColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.ambientColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DiffuseColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.diffuseColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::SpecularColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.specularColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::EmissiveColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.emissiveColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::TextureTileFactor:
				{
					float temp = mergedMaterialProperties.textureTileFactor;
					std::memcpy(buffer + descriptor->GetOffset(), &temp, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::AlphaToCoverageClamp:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.alphaToCoverageClamp.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraPosition:
				{
					RN::Vector3 cameraPosition = renderPass.viewPosition;
					std::memcpy(buffer + descriptor->GetOffset(), &cameraPosition.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraAmbientColor:
				{
					RN::Color cameraAmbientColor = renderPass.cameraAmbientColor;
					std::memcpy(buffer + descriptor->GetOffset(), &cameraAmbientColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraFogColor0:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraFogColor0.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraFogColor1:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraFogColor1.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraClipDistance:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraClipDistance.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraFogDistance:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraFogDistance.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLightsCount:
				{
					uint32 lightCount = std::min(renderPass.directionalLights.size(), descriptor->GetElementCount());
					std::memcpy(buffer + descriptor->GetOffset(), &lightCount, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLights:
				{
					size_t lightCount = std::min(renderPass.directionalLights.size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalLights[0], (16 + 16) * lightCount);
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatricesCount:
				{
					//TODO: Limit matrixCount to descriptor->GetElementCount() of Shader::UniformDescriptor::Identifier::DirectionalShadowMatrices
					uint32 matrixCount = renderPass.directionalShadowMatrices.size();
					std::memcpy(buffer + descriptor->GetOffset(), &matrixCount, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatrices:
				{
					uint32 matrixCount = std::min(renderPass.directionalShadowMatrices.size(), descriptor->GetElementCount());
					if(matrixCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalShadowMatrices[0].m[0], 64 * matrixCount);
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowInfo:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalShadowInfo.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::PointLights:
				{
					uint32 lightCount = std::min(renderPass.pointLights.size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.pointLights[0], (12 + 4 + 16) * lightCount);
					}
					if(lightCount < 8)
					{
						std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 16) * lightCount, 0, (12 + 4 + 16) * (descriptor->GetElementCount() - lightCount));
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::SpotLights:
				{
					uint32 lightCount = std::min(renderPass.spotLights.size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.spotLights[0], (12 + 4 + 12 + 4 + 16) * lightCount);
					}
					if(lightCount < 8)
					{
						std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 12 + 4 + 16) * lightCount, 0, (12 + 4 + 12 + 4 + 16) * (descriptor->GetElementCount() - lightCount));
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::BoneMatrices:
				{
					if(drawable->skeleton)
					{
						uint32 matrixCount = std::min(drawable->skeleton->_matrices.size(), descriptor->GetElementCount());
						if(matrixCount > 0)
						{
							std::memcpy(buffer + descriptor->GetOffset(), &drawable->skeleton->_matrices[0].m[0], 64 * matrixCount);
						}
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::Custom:
				{
					Object *object = mergedMaterialProperties.GetCustomShaderUniform(descriptor->GetName());
					if(object)
					{
						if(object->IsKindOfClass(Value::GetMetaClass()))
						{
							Value *value = object->Downcast<Value>();
							switch(value->GetValueType())
							{
								case TypeTranslator<Vector2>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector2))
									{
										Vector2 vector = value->GetValue<Vector2>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Vector3>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector3))
									{
										Vector3 vector = value->GetValue<Vector3>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Vector4>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector4))
									{
										Vector4 vector = value->GetValue<Vector4>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Matrix>::value:
								{
									if(descriptor->GetSize() == sizeof(Matrix))
									{
										Matrix matrix = value->GetValue<Matrix>();
										std::memcpy(buffer + descriptor->GetOffset(), &matrix.m[0], descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Quaternion>::value:
								{
									if(descriptor->GetSize() == sizeof(Quaternion))
									{
										Quaternion quaternion = value->GetValue<Quaternion>();
										std::memcpy(buffer + descriptor->GetOffset(), &quaternion.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Color>::value:
								{
									if(descriptor->GetSize() == sizeof(Color))
									{
										Color color = value->GetValue<Color>();
										std::memcpy(buffer + descriptor->GetOffset(), &color.r, descriptor->GetSize());
									}
									break;
								}
								default:
									break;
							}
						}
						else
						{
							Number *number = object->Downcast<Number>();
							switch(number->GetType())
							{
								case Number::Type::Int8:
								{
									if(descriptor->GetSize() == sizeof(int8))
									{
										int8 value = number->GetInt8Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Int16:
								{
									if(descriptor->GetSize() == sizeof(int8))
									{
										int16 value = number->GetInt16Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Int32:
								{
									if(descriptor->GetSize() == sizeof(int32))
									{
										int32 value = number->GetInt32Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint8:
								{
									if(descriptor->GetSize() == sizeof(uint8))
									{
										uint8 value = number->GetUint8Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint16:
								{
									if(descriptor->GetSize() == sizeof(uint16))
									{
										uint16 value = number->GetUint16Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint32:
								{
									if(descriptor->GetSize() == sizeof(uint32))
									{
										uint32 value = number->GetUint32Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Float32:
								{
									if(descriptor->GetSize() == sizeof(float))
									{
										float value = number->GetFloatValue();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Boolean:
								{
									if(descriptor->GetSize() == sizeof(bool))
									{
										bool value = number->GetBoolValue();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								default:
									break;
							}
						}
					}
					break;
				}

				default:
					break;
			}
		});
	}

	void D3D12Renderer::SubmitLight(const Light *light)
	{
		_lock.Lock();
		D3D12RenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		_lock.Unlock();

		if(light->GetType() == Light::Type::DirectionalLight)
		{
			renderPass.directionalLights.push_back(D3D12LightDirectional{ light->GetForward(), 0.0f, light->GetFinalColor() });

			//TODO: Allow more lights with shadows or prevent multiple light with shadows overwriting each other
			if(light->HasShadows())
			{
				renderPass.directionalShadowDepthTexture = light->GetShadowDepthTexture()->Downcast<D3D12Texture>();
				renderPass.directionalShadowMatrices = light->GetShadowMatrices();
				renderPass.directionalShadowInfo = Vector2(1.0f / light->GetShadowParameters().resolution);
			}
		}
		else if(light->GetType() == Light::Type::PointLight)
		{
			renderPass.pointLights.push_back(VulkanPointLight{ light->GetWorldPosition(), light->GetRange(), light->GetFinalColor() });
		}
		else if(light->GetType() == Light::Type::SpotLight)
		{
			renderPass.spotLights.push_back(VulkanSpotLight{ light->GetWorldPosition(), light->GetRange(), light->GetForward(), light->GetAngleCos(), light->GetFinalColor() });
		}
	}

	void D3D12Renderer::SubmitDrawable(Drawable *tdrawable)
	{
		D3D12Drawable *drawable = static_cast<D3D12Drawable *>(tdrawable);
		drawable->AddUniformStateIfNeeded(_internals->currentDrawableResourceIndex);

		_lock.Lock();
		D3D12RenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		_lock.Unlock();

		if(drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].dirty)
		{
			//TODO: Fix the camera situation...
			Material *material = drawable->material;
			_lock.Lock();
			const D3D12PipelineState *pipelineState = _internals->stateCoordinator.GetRenderPipelineState(material, drawable->mesh, renderPass.framebuffer, renderPass.shaderHint, renderPass.overrideMaterial);
			D3D12UniformState *uniformState = _internals->stateCoordinator.GetUniformStateForPipelineState(pipelineState);
			_lock.Unlock();

			RN_ASSERT(pipelineState && uniformState, "Failed to create pipeline or uniform state for drawable!");
			drawable->UpdateRenderingState(_internals->currentDrawableResourceIndex, pipelineState, uniformState);
		}

		//Vertex and fragment shaders need to explicitly be marked to support instancing in the shader library json
		RN::Shader *vertexShader = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->descriptor.vertexShader;
		RN::Shader *fragmentShader = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->descriptor.fragmentShader;
		bool canUseInstancing = (!vertexShader || vertexShader->GetHasInstancing()) && (!fragmentShader || fragmentShader->GetHasInstancing());

		//Check if uniform buffers are the same, the object can't be part of the same instanced draw call if it doesn't share the same buffers (because they are full for example)
		if(canUseInstancing && _internals->currentInstanceDrawable && drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->vertexUniformBuffers.size() == _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->vertexUniformBuffers.size() && drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->fragmentUniformBuffers.size() == _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->fragmentUniformBuffers.size())
		{
			canUseInstancing = true;
			size_t vertexUniformBufferCount = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->vertexUniformBuffers.size();
			for(int i = 0; i < vertexUniformBufferCount; i++)
			{
				if(drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->vertexUniformBuffers[i]->uniformBuffer != _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->vertexUniformBuffers[i]->uniformBuffer)
				{
					canUseInstancing = false;
					break;
				}
			}

			size_t fragmentUniformBufferCount = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->fragmentUniformBuffers.size();
			for(int i = 0; i < fragmentUniformBufferCount; i++)
			{
				if(drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->fragmentUniformBuffers[i]->uniformBuffer != _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState->fragmentUniformBuffers[i]->uniformBuffer)
				{
					canUseInstancing = false;
					break;
				}
			}
		}

		if(canUseInstancing && renderPass.instanceSteps.size() > 0 && renderPass.instanceSteps.back() >= std::min(vertexShader->GetMaxInstanceCount(), fragmentShader ? fragmentShader->GetMaxInstanceCount() : -1))
		{
			canUseInstancing = false;
		}

		_lock.Lock();
		if(_internals->currentPipelineState && _internals->currentPipelineState->state == drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->state && drawable->mesh == _internals->currentInstanceDrawable->mesh && drawable->material->GetTextures()->IsEqual(_internals->currentInstanceDrawable->material->GetTextures()) && canUseInstancing)
		{
			renderPass.instanceSteps.back() += 1; //Increase counter if the rendering state is the same
		}
		else
		{
			_internals->currentPipelineState = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState;
			_internals->currentInstanceDrawable = drawable;
			renderPass.instanceSteps.push_back(1); //Add new entry if the rendering state changed
			
			_internals->totalDescriptorTables += drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature->vertexTextureCount;
			_internals->totalDescriptorTables += drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature->fragmentTextureCount;
			_internals->totalDescriptorTables += drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature->vertexUniformBufferCount;
			_internals->totalDescriptorTables += drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature->fragmentUniformBufferCount;
		}

		// Push into the queue
		renderPass.drawables.push_back(drawable);
		_lock.Unlock();
	}

	void D3D12Renderer::RenderDrawable(ID3D12GraphicsCommandList *commandList, D3D12Drawable *drawable, uint32 instanceCount)
	{
		const D3D12RootSignature *rootSignature = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature;
		UINT rootParameter = 0;
		
		if(rootSignature->vertexTextureCount || rootSignature->vertexUniformBufferCount)
		{
			commandList->SetGraphicsRootDescriptorTable(rootParameter++, _currentSrvCbvHeap->GetGPUHandle(_currentSrvCbvIndex));
			_currentSrvCbvIndex += rootSignature->vertexTextureCount + rootSignature->vertexUniformBufferCount;
		}
		
		if(rootSignature->fragmentTextureCount || rootSignature->fragmentUniformBufferCount)
		{
			commandList->SetGraphicsRootDescriptorTable(rootParameter++, _currentSrvCbvHeap->GetGPUHandle(_currentSrvCbvIndex));
			_currentSrvCbvIndex += rootSignature->fragmentTextureCount + rootSignature->fragmentUniformBufferCount;
		}

		commandList->SetPipelineState(drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->state);

		D3D12GPUBuffer *buffer = static_cast<D3D12GPUBuffer *>(drawable->mesh->GetGPUVertexBuffer());
		D3D12GPUBuffer *indices = static_cast<D3D12GPUBuffer *>(drawable->mesh->GetGPUIndicesBuffer());

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[2];
		vertexBufferViews[0].BufferLocation = buffer->GetD3D12Resource()->GetGPUVirtualAddress();
		vertexBufferViews[0].StrideInBytes = drawable->mesh->GetVertexPositionsSeparatedSize() > 0? drawable->mesh->GetVertexPositionsSeparatedStride() : drawable->mesh->GetStride();
		vertexBufferViews[0].SizeInBytes = drawable->mesh->GetVertexPositionsSeparatedSize() > 0? drawable->mesh->GetVertexPositionsSeparatedSize() : buffer->GetLength();

		vertexBufferViews[1].BufferLocation = buffer->GetD3D12Resource()->GetGPUVirtualAddress() + drawable->mesh->GetVertexPositionsSeparatedSize();
		vertexBufferViews[1].StrideInBytes = drawable->mesh->GetStride();
		vertexBufferViews[1].SizeInBytes = buffer->GetLength() - drawable->mesh->GetVertexPositionsSeparatedSize();

		commandList->IASetVertexBuffers(0, (drawable->mesh->GetVertexPositionsSeparatedSize() > 0 && drawable->mesh->GetVertexPositionsSeparatedSize() < buffer->GetLength())? 2 : 1, vertexBufferViews);

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = indices->GetD3D12Resource()->GetGPUVirtualAddress();
		indexBufferView.Format = drawable->mesh->GetAttribute(Mesh::VertexAttribute::Feature::Indices)->GetType() == PrimitiveType::Uint16? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = indices->GetLength();
		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->DrawIndexedInstanced(drawable->mesh->GetIndicesCount(), instanceCount, 0, 0, 0);

		_currentDrawableIndex += 1;
	}

	void D3D12Renderer::RenderAPIRenderPass(D3D12CommandList *commandList, const D3D12RenderPass &renderPass)
	{
		//TODO: Handle multiple and not existing textures
		Texture *sourceColorTexture = renderPass.previousRenderPass->GetFramebuffer()->GetColorTexture(0);
		D3D12Texture *sourceD3DColorTexture = nullptr;
		D3D12_RESOURCE_STATES oldColorSourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		if(sourceColorTexture)
		{
			sourceD3DColorTexture = sourceColorTexture->Downcast<D3D12Texture>();
			oldColorSourceState = sourceD3DColorTexture->_currentState;
		}

		Texture *sourceDepthTexture = renderPass.previousRenderPass->GetFramebuffer()->GetDepthStencilTexture();
		D3D12Texture *sourceD3DDepthTexture = nullptr;
		D3D12_RESOURCE_STATES oldDepthSourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		if(sourceDepthTexture)
		{
			sourceD3DDepthTexture = sourceDepthTexture->Downcast<D3D12Texture>();
			oldDepthSourceState = sourceD3DDepthTexture->_currentState;
		}

		D3D12Framebuffer *destinationFramebuffer = renderPass.renderPass->GetFramebuffer()->Downcast<RN::D3D12Framebuffer>();

		Texture *destinationColorTexture = destinationFramebuffer->GetColorTexture(0);
		D3D12Texture *destinationD3DColorTexture = nullptr;
		ID3D12Resource *destinationColorResource = nullptr;
		D3D12_RESOURCE_STATES oldColorDestinationState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		DXGI_FORMAT targetColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		if(destinationColorTexture)
		{
			destinationD3DColorTexture = destinationColorTexture->Downcast<D3D12Texture>();
			targetColorFormat = destinationD3DColorTexture->_srvDescriptor.Format;
			oldColorDestinationState = destinationD3DColorTexture->_currentState;
			destinationColorResource = destinationD3DColorTexture->_resource;
		}
		else
		{
			targetColorFormat = destinationFramebuffer->_colorTargets[0]->d3dTargetViewDesc.Format;
			destinationColorResource = destinationFramebuffer->GetSwapChainColorBuffer();
		}


		Texture *destinationDepthTexture = destinationFramebuffer->GetDepthStencilTexture();
		D3D12Texture *destinationD3DDepthTexture = nullptr;
		ID3D12Resource *destinationDepthResource = nullptr;
		D3D12_RESOURCE_STATES oldDepthDestinationState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		DXGI_FORMAT targetDepthFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		if(destinationDepthTexture)
		{
			destinationD3DDepthTexture = destinationDepthTexture->Downcast<D3D12Texture>();
			targetDepthFormat = destinationD3DDepthTexture->_srvDescriptor.Format;
			oldDepthDestinationState = destinationD3DDepthTexture->_currentState;
			destinationDepthResource = destinationD3DDepthTexture->_resource;
		}
		else if(destinationFramebuffer->GetSwapChain() && destinationFramebuffer->GetSwapChain()->HasDepthBuffer())
		{
			targetDepthFormat = destinationFramebuffer->_depthStencilTarget->d3dTargetViewDesc.Format;
			destinationDepthResource = destinationFramebuffer->GetSwapChainDepthBuffer();
		}

		switch(targetDepthFormat)
		{
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			{
				targetDepthFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			}
			case DXGI_FORMAT_D32_FLOAT:
			{
				targetDepthFormat = DXGI_FORMAT_R32_FLOAT;
				break;
			}
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			{
				targetDepthFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			}
		}

		if(renderPass.type == D3D12RenderPass::Type::ResolveMSAA)
		{
			sourceD3DColorTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

			if(destinationColorTexture)
			{
				destinationD3DColorTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_RESOLVE_DEST);
			}
			else
			{
				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationColorResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_DEST));
			}

			//TODO: Handle multiple subresources?
			for(uint32 i = 0; i < sourceD3DColorTexture->_descriptor.depth; i++)
			{
				uint32 subresourceIndex = D3D12CalcSubresource(0, i, 0, sourceD3DColorTexture->_descriptor.mipMaps, sourceD3DColorTexture->_descriptor.depth);
				commandList->GetCommandList()->ResolveSubresource(destinationColorResource, subresourceIndex, sourceD3DColorTexture->_resource, subresourceIndex, targetColorFormat);
			}

			sourceD3DColorTexture->TransitionToState(commandList, oldColorSourceState);
			if(destinationD3DColorTexture)
			{
				destinationD3DColorTexture->TransitionToState(commandList, oldColorDestinationState);
			}
			else
			{
				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationColorResource, D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));
			}

			if(sourceD3DDepthTexture && destinationDepthResource)
			{
				sourceD3DDepthTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

				if(destinationDepthTexture)
				{
					destinationD3DDepthTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_RESOLVE_DEST);
				}
				else
				{
					commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationDepthResource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_RESOLVE_DEST));
				}

				//TODO: Handle multiple subresources?
				for(uint32 i = 0; i < sourceD3DDepthTexture->_descriptor.depth; i++)
				{
					uint32 subresourceIndex = D3D12CalcSubresource(0, i, 0, sourceD3DDepthTexture->_descriptor.mipMaps, sourceD3DDepthTexture->_descriptor.depth);
					commandList->GetCommandList()->ResolveSubresource(destinationDepthResource, subresourceIndex, sourceD3DDepthTexture->_resource, subresourceIndex, targetDepthFormat);
				}

				sourceD3DDepthTexture->TransitionToState(commandList, oldDepthSourceState);
				if(destinationD3DDepthTexture)
				{
					destinationD3DDepthTexture->TransitionToState(commandList, oldDepthDestinationState);
				}
				else
				{
					commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationDepthResource, D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_DEPTH_WRITE));
				}
			}
		}
		else if(renderPass.type == D3D12RenderPass::Type::Blit)
		{
			sourceD3DColorTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE);

			if(destinationColorTexture)
			{
				destinationD3DColorTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
			}
			else
			{
				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationColorResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST));
			}

			//TODO: Handle multiple subresources and 3D/Arrays?
			CD3DX12_TEXTURE_COPY_LOCATION destinationLocation(destinationColorResource, 0);
			CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(sourceD3DColorTexture->_resource, 0);
			Rect frame = renderPass.renderPass->GetFrame();
			commandList->GetCommandList()->CopyTextureRegion(&destinationLocation, frame.x, frame.y, 0, &sourceLocation, nullptr);

			sourceD3DColorTexture->TransitionToState(commandList, oldColorSourceState);
			if(destinationD3DColorTexture)
			{
				destinationD3DColorTexture->TransitionToState(commandList, oldColorDestinationState);
			}
			else
			{
				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationColorResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));
			}

			if(sourceD3DDepthTexture && destinationDepthResource)
			{
				sourceD3DDepthTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE);

				if(destinationDepthTexture)
				{
					destinationD3DDepthTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
				}
				else
				{
					commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationDepthResource, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_DEST));
				}

				//TODO: Handle multiple subresources and 3D/Arrays?
				CD3DX12_TEXTURE_COPY_LOCATION destinationLocation(destinationDepthResource, 0);
				CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(sourceD3DDepthTexture->_resource, 0);
				Rect frame = renderPass.renderPass->GetFrame();
				commandList->GetCommandList()->CopyTextureRegion(&destinationLocation, frame.x, frame.y, 0, &sourceLocation, nullptr);

				sourceD3DDepthTexture->TransitionToState(commandList, oldDepthSourceState);
				if (destinationD3DDepthTexture)
				{
					destinationD3DDepthTexture->TransitionToState(commandList, oldDepthDestinationState);
				}
				else
				{
					commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationDepthResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_DEPTH_WRITE));
				}
			}
		}
	}

	void D3D12Renderer::AddFrameResouce(IUnknown *resource)
	{
		_internals->frameResources.push_back({ resource, _scheduledFenceValue });
	}
}

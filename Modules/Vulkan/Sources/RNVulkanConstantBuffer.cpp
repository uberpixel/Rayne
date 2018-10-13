//
//  RNVulkanConstantBuffer.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanConstantBuffer.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanGPUBuffer.h"

namespace RN
{
	RNDefineMeta(VulkanConstantBuffer, Object)
	RNDefineMeta(VulkanConstantBufferReference, Object)

	VulkanConstantBuffer::VulkanConstantBuffer(Renderer *renderer, size_t size) :
		_bufferIndex(0),
		_sizeUsed(0),
		_offsetToFreeData(0),
		_totalSize(size)
	{
		VulkanRenderer *realRenderer = renderer->Downcast<VulkanRenderer>();
		GPUBuffer *buffer = realRenderer->CreateBufferWithLength(size, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);
		_buffers.push_back(buffer);
		_bufferFrames.push_back(0);
	}

	VulkanConstantBuffer::~VulkanConstantBuffer()
	{
		for(size_t i = 0; i < _buffers.size(); i ++)
			_buffers[i]->Release();
	}

	GPUBuffer *VulkanConstantBuffer::Advance(size_t currentFrame, size_t completedFrame)
	{
		_bufferIndex = (_bufferIndex + 1) % _buffers.size();

		if(_bufferFrames[_bufferIndex] > completedFrame)
		{
			VulkanRenderer *realRenderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
			GPUBuffer *buffer = realRenderer->CreateBufferWithLength(_totalSize, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);

			_bufferIndex += 1;
			if(_bufferIndex >= _buffers.size())
			{
				_buffers.push_back(buffer);
				_bufferFrames.push_back(currentFrame);
			}
			else
			{
				_buffers.insert(_buffers.begin() + _bufferIndex, buffer);
				_bufferFrames.insert(_bufferFrames.begin() + _bufferIndex, currentFrame);
			}
		}
		else
		{
			_bufferFrames[_bufferIndex] = currentFrame;
		}

		return _buffers[_bufferIndex];
	}

	size_t VulkanConstantBuffer::Allocate(size_t size)
	{
		if(_totalSize - _offsetToFreeData < size)
			return -1;

		size_t newDataOffset = _offsetToFreeData;
		_offsetToFreeData += size;
		_offsetToFreeData += kRNConstantBufferAlignement - (_offsetToFreeData % kRNConstantBufferAlignement);
		_sizeUsed += size;
		return newDataOffset;
	}

	void VulkanConstantBuffer::Free(size_t offset, size_t size)
	{
		_sizeUsed -= size;
		if(_sizeUsed <= 0)
		{
			_offsetToFreeData = 0;
		}
	}

	VulkanConstantBufferReference::VulkanConstantBufferReference() : shaderResourceIndex(0), offset(0), size(0), constantBuffer(nullptr)
	{

	}

	VulkanConstantBufferReference::~VulkanConstantBufferReference()
	{
		constantBuffer->Free(offset, size);
	}

	VulkanConstantBufferPool::VulkanConstantBufferPool() :
		_constantBuffers(new Array()),
		_newReferences(new Array())
	{

	}

	VulkanConstantBufferPool::~VulkanConstantBufferPool()
	{
		_constantBuffers->Release();
		_newReferences->Release();
	}

	VulkanConstantBufferReference *VulkanConstantBufferPool::GetConstantBufferReference(uint32 size, uint32 index)
	{
		size_t bufferOffset = -1;
		VulkanConstantBuffer *uniformBuffer = nullptr;
		_constantBuffers->Enumerate<VulkanConstantBuffer>([&](VulkanConstantBuffer *buffer, uint32 index, bool &stop){
			bufferOffset = buffer->Allocate(size);
			if(bufferOffset != -1)
			{
				uniformBuffer = buffer;
				stop = true;
			}
		});

		VulkanConstantBufferReference *reference = new VulkanConstantBufferReference();
		reference->size = size;
		reference->offset = bufferOffset;
		reference->constantBuffer = uniformBuffer;
		reference->shaderResourceIndex = index;

		if(uniformBuffer) return reference->Autorelease();

		_newReferences->AddObject(reference);
		return reference->Autorelease();
	}

	void VulkanConstantBufferPool::Update(Renderer *renderer, size_t currentFrame, size_t completedFrame)
	{
		_constantBuffers->Enumerate<VulkanConstantBuffer>([&](VulkanConstantBuffer *buffer, uint32 index, bool &stop){
			buffer->Advance(currentFrame, completedFrame);
		});

		size_t requiredSize = 0;
		_newReferences->Enumerate<VulkanConstantBufferReference>([&](VulkanConstantBufferReference *reference, uint32 index, bool &stop){
			requiredSize += reference->size + kRNConstantBufferAlignement - (reference->size % kRNConstantBufferAlignement);
		});

		if(requiredSize == 0) return;

		requiredSize = std::max(requiredSize, static_cast<size_t>(kRNMinimumConstantBufferSize));
		VulkanConstantBuffer *constantBuffer = new VulkanConstantBuffer(renderer, requiredSize);

		_newReferences->Enumerate<VulkanConstantBufferReference>([&](VulkanConstantBufferReference *reference, uint32 index, bool &stop){
			reference->constantBuffer = constantBuffer;
			reference->offset = constantBuffer->Allocate(reference->size);
		});

		_constantBuffers->AddObject(constantBuffer->Autorelease());
		_newReferences->RemoveAllObjects();
	}

	void VulkanConstantBufferPool::InvalidateAllBuffers()
	{
		_constantBuffers->Enumerate<VulkanConstantBuffer>([&](VulkanConstantBuffer *buffer, uint32 index, bool &stop){
			if(buffer->_sizeUsed > 0)
				buffer->GetActiveBuffer()->Invalidate();
		});
	}
}

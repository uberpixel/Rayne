//
//  RNMetalUniformBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalUniformBuffer.h"
#include "RNMetalInternals.h"
#include "RNMetalRenderer.h"

namespace RN
{
	RNDefineMeta(MetalUniformBuffer, Object)
	RNDefineMeta(MetalUniformBufferReference, Object)

	MetalUniformBuffer::MetalUniformBuffer(Renderer *renderer, size_t size) :
		_bufferIndex(0),
		_sizeUsed(0),
		_offsetToFreeData(0),
		_totalSize(size)
	{
		//This seems to be working as alignement, but could cause problems with arrays of structs in the buffer
		if((_totalSize % 16) > 0)
			_totalSize += 16 - (_totalSize % 16);
		
		for(size_t i = 0; i < kRNMetalUniformBufferCount; i++)
			_buffers[i] = renderer->CreateBufferWithLength(_totalSize, GPUResource::UsageOptions::Uniform , GPUResource::AccessOptions::WriteOnly);
	}

	MetalUniformBuffer::~MetalUniformBuffer()
	{
		for(size_t i = 0; i < kRNMetalUniformBufferCount; i++)
			_buffers[i]->Release();
	}

	GPUBuffer *MetalUniformBuffer::Advance()
	{
		_bufferIndex = (_bufferIndex + 1) % kRNMetalUniformBufferCount;
		return _buffers[_bufferIndex];
	}
	
	size_t MetalUniformBuffer::Allocate(size_t size)
	{
		if(_totalSize - _offsetToFreeData < size)
			return -1;
		
		size_t newDataOffset = _offsetToFreeData;
		_offsetToFreeData += size;
		_offsetToFreeData += kRNUniformBufferAlignement - (_offsetToFreeData % kRNUniformBufferAlignement);
		_sizeUsed += size;
		return newDataOffset;
	}
	
	void MetalUniformBuffer::Free(size_t offset, size_t size)
	{
		_sizeUsed -= size;
		if(_sizeUsed <= 0)
		{
			_offsetToFreeData = 0;
		}
	}
	
	MetalUniformBufferReference::MetalUniformBufferReference()
	{
		
	}
	
	MetalUniformBufferReference::~MetalUniformBufferReference()
	{
		uniformBuffer->Free(offset, size);
	}
	
	MetalUniformBufferPool::MetalUniformBufferPool() :
		_uniformBuffers(new Array()),
		_newReferences(new Array())
	{
		
	}
	
	MetalUniformBufferPool::~MetalUniformBufferPool()
	{
		_uniformBuffers->Release();
		_newReferences->Release();
	}
	
	MetalUniformBufferReference *MetalUniformBufferPool::GetUniformBufferReference(uint32 size, uint32 index)
	{
		size_t bufferOffset = -1;
		MetalUniformBuffer *uniformBuffer = nullptr;
		_uniformBuffers->Enumerate<MetalUniformBuffer>([&](MetalUniformBuffer *buffer, uint32 index, bool &stop){
			bufferOffset = buffer->Allocate(size);
			if(bufferOffset != -1)
			{
				uniformBuffer = buffer;
				stop = true;
			}
		});
		
		MetalUniformBufferReference *reference = new MetalUniformBufferReference();
		reference->size = size;
		reference->offset = bufferOffset;
		reference->uniformBuffer = uniformBuffer;
		reference->shaderResourceIndex = index;
		
		if(uniformBuffer) return reference->Autorelease();
		
		_newReferences->AddObject(reference);
		return reference->Autorelease();
	}
	
	void MetalUniformBufferPool::Update(Renderer *renderer)
	{
		_uniformBuffers->Enumerate<MetalUniformBuffer>([&](MetalUniformBuffer *buffer, uint32 index, bool &stop){
			buffer->Advance();
		});
		
		size_t requiredSize = 0;
		_newReferences->Enumerate<MetalUniformBufferReference>([&](MetalUniformBufferReference *reference, uint32 index, bool &stop){
			requiredSize += reference->size + kRNUniformBufferAlignement - (reference->size % kRNUniformBufferAlignement);
		});
		
		if(requiredSize == 0) return;
		
		requiredSize = MAX(requiredSize, kRNMinimumUniformBufferSize);
		MetalUniformBuffer *uniformBuffer = new MetalUniformBuffer(renderer, requiredSize);
		
		_newReferences->Enumerate<MetalUniformBufferReference>([&](MetalUniformBufferReference *reference, uint32 index, bool &stop){
			reference->uniformBuffer = uniformBuffer;
			reference->offset = uniformBuffer->Allocate(reference->size);
		});
		
		_uniformBuffers->AddObject(uniformBuffer->Autorelease());
		_newReferences->RemoveAllObjects();
	}
	
	void MetalUniformBufferPool::InvalidateAllBuffers()
	{
		_uniformBuffers->Enumerate<MetalUniformBuffer>([&](MetalUniformBuffer *buffer, uint32 index, bool &stop){
			if(buffer->_sizeUsed > 0)
				buffer->GetActiveBuffer()->Invalidate();
		});
	}
}

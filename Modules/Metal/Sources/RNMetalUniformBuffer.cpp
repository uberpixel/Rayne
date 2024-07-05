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
		_totalSize(size),
		_sizeReserved(0)
	{
		//This seems to be working as alignement, but could cause problems with arrays of structs in the buffer
		if((_totalSize % 16) > 0)
			_totalSize += 16 - (_totalSize % 16);
		
		for(size_t i = 0; i < kRNMetalUniformBufferCount; i++)
			_buffers[i] = renderer->CreateBufferWithLength(_totalSize, GPUResource::UsageOptions::Uniform , GPUResource::AccessOptions::WriteOnly, false);
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

	void MetalUniformBuffer::Reset()
	{
		//Doesn't actually remove any data, just resets the allocation info to start allocating from the start again.
		_sizeUsed = 0;
		_offsetToFreeData = 0;
	}
	
	size_t MetalUniformBuffer::Allocate(size_t size, bool align)
	{
		//Align offset when allocating the next buffer (if it is supposed to be aligned)
		if(align) _offsetToFreeData += kRNUniformBufferAlignement - (_offsetToFreeData % kRNUniformBufferAlignement);
		
		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_offsetToFreeData);
		if(availableSize < static_cast<int>(size))
			return -1;
		
		size_t newDataOffset = _offsetToFreeData;
		_offsetToFreeData += size;
		_sizeUsed += size;
		return newDataOffset;
	}

	size_t MetalUniformBuffer::Reserve(size_t size)
	{
		size_t alignedSize = size + kRNUniformBufferAlignement - (size % kRNUniformBufferAlignement);
		
		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_sizeReserved);
		if(availableSize < static_cast<int>(alignedSize))
			return -1;
		
		_sizeReserved += alignedSize;
		return alignedSize;
	}

	void MetalUniformBuffer::Unreserve(size_t size)
	{
		_sizeReserved -= size;
	}
	
	MetalUniformBufferReference::MetalUniformBufferReference() : uniformBuffer(nullptr)
	{
		
	}
	
	MetalUniformBufferReference::~MetalUniformBufferReference()
	{
		if(uniformBuffer)
		{
			uniformBuffer->Unreserve(reservedSize);
		}
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
		MetalUniformBuffer *uniformBuffer = nullptr;
		size_t reservedSize = 0;
		_uniformBuffers->Enumerate<MetalUniformBuffer>([&](MetalUniformBuffer *buffer, uint32 index, bool &stop){
			reservedSize = buffer->Reserve(size);
			if(reservedSize != -1)
			{
				uniformBuffer = buffer;
				stop = true;
			}
		});
		
		MetalUniformBufferReference *reference = new MetalUniformBufferReference();
		reference->size = size;
		reference->reservedSize = reservedSize;
		reference->offset = -1;
		reference->uniformBuffer = uniformBuffer;
		reference->shaderResourceIndex = index;
		
		if(uniformBuffer) return reference->Autorelease(); //return directly if a uniform buffer already exists for this one
		
		//In this case there is no big enough uniform buffer yet and it needs to be allocated first, still remove a reference though, it will be assigned the buffer later
		_newReferences->AddObject(reference);
		return reference->Autorelease();
	}

	void MetalUniformBufferPool::UpdateUniformBufferReference(MetalUniformBufferReference *reference, bool align)
	{
		RN_ASSERT(reference->uniformBuffer, "Somethings up with the reference not having a uniform buffer assigned");
		size_t bufferOffset = reference->uniformBuffer->Allocate(reference->size, align);
		RN_ASSERT(bufferOffset != -1, "The uniform buffer does not have enough space to fit the memory required by this reference. This should never happen...");
		
		reference->offset = bufferOffset;
	}
	
	void MetalUniformBufferPool::Update(Renderer *renderer)
	{
		_uniformBuffers->Enumerate<MetalUniformBuffer>([&](MetalUniformBuffer *buffer, uint32 index, bool &stop){
			buffer->Advance();
			buffer->Reset();
		});
		
		if(_newReferences->GetCount() == 0) return;
		
		while(1)
		{
			size_t requiredSize = 0;
			size_t lastIndexToRemove = 0;
			_newReferences->Enumerate<MetalUniformBufferReference>([&](MetalUniformBufferReference *reference, uint32 index, bool &stop){
				//Not all will actually need alignment if instancing is used, but better to overallocate here
				size_t sizeToAdd = reference->size + kRNUniformBufferAlignement - (reference->size % kRNUniformBufferAlignement);
				if(requiredSize + sizeToAdd <= kRNMaximumUniformBufferSize)
				{
					requiredSize += sizeToAdd;
					lastIndexToRemove = index;
				}
				else
				{
					stop = true;
				}
			});
		
			if(requiredSize == 0) break;
			requiredSize = MAX(requiredSize, kRNMinimumUniformBufferSize);
			
			//Create a new uniform buffer
			MetalUniformBuffer *uniformBuffer = new MetalUniformBuffer(renderer, requiredSize);
			
			for(int32 i = lastIndexToRemove; i >= 0; i--)
			{
				MetalUniformBufferReference *reference = _newReferences->GetObjectAtIndex<MetalUniformBufferReference>(i);
				reference->uniformBuffer = uniformBuffer;
				reference->reservedSize = uniformBuffer->Reserve(reference->size);
				
				//Remove already handled references, to not allocate another buffer (if the current required allocation was too big for a single buffer)
				_newReferences->RemoveObjectAtIndex(i);
			}
			
			_uniformBuffers->AddObject(uniformBuffer->Autorelease());
		}
		_newReferences->RemoveAllObjects();
	}
	
	void MetalUniformBufferPool::FlushAllBuffers()
	{
		_uniformBuffers->Enumerate<MetalUniformBuffer>([&](MetalUniformBuffer *buffer, uint32 index, bool &stop){
			if(buffer->_sizeUsed > 0)
				buffer->GetActiveBuffer()->Flush();
		});
	}
}

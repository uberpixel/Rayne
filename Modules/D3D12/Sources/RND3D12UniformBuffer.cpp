//
//  RND3D12UniformBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12UniformBuffer.h"
#include "RND3D12Renderer.h"
#include "RND3D12GPUBuffer.h"

namespace RN
{
	RNDefineMeta(D3D12UniformBuffer, Object)

	D3D12UniformBuffer::D3D12UniformBuffer(Renderer *renderer, size_t size) :
		_bufferIndex(0),
		_supportedFeatures(0)
	{
		D3D12Renderer *realRenderer = renderer->Downcast<D3D12Renderer>();

		//Three buffers for triplebuffering
		for(size_t i = 0; i < kRND3D12UniformBufferCount; i++)
		{
			_buffers[i] = renderer->CreateBufferWithLength(size, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = _buffers[i]->Downcast<D3D12GPUBuffer>()->GetD3D12Buffer()->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = _buffers[i]->GetLength();
			realRenderer->GetD3D12Device()->GetDevice()->CreateConstantBufferView(&cbvDesc, realRenderer->GetUniformDescriptorCPUHandle(i));
		}

/*		AutoreleasePool pool;

		for(D3D12RenderingStateUniformBufferMember *member : uniformBuffer->members)
		{
			const String *name = member->GetName();
			size_t offset = member->GetOffset();

			if(name->IsEqual(RNCSTR("time")))
				_members.emplace_back(Feature::Time, offset);
			else if(name->IsEqual(RNCSTR("modelMatrix")))
				_members.emplace_back(Feature::ModelMatrix, offset);
			else if(name->IsEqual(RNCSTR("modelViewMatrix")))
				_members.emplace_back(Feature::ModelViewMatrix, offset);
			else if(name->IsEqual(RNCSTR("modelViewProjectionMatrix")))
				_members.emplace_back(Feature::ModelViewProjectionMatrix, offset);
			else if(name->IsEqual(RNCSTR("viewMatrix")))
				_members.emplace_back(Feature::ViewMatrix, offset);
			else if(name->IsEqual(RNCSTR("viewProjectionMatrix")))
				_members.emplace_back(Feature::ViewProjectionMatrix, offset);
			else if(name->IsEqual(RNCSTR("projectionMatrix")))
				_members.emplace_back(Feature::ProjectionMatrix, offset);
			else if(name->IsEqual(RNCSTR("inverseModelMatrix")))
				_members.emplace_back(Feature::InverseModelMatrix, offset);
			else if(name->IsEqual(RNCSTR("inverseModelViewMatrix")))
				_members.emplace_back(Feature::InverseModelViewMatrix, offset);
			else if(name->IsEqual(RNCSTR("inverseModelViewProjectionMatrix")))
				_members.emplace_back(Feature::InverseModelViewProjectionMatrix, offset);
			else if(name->IsEqual(RNCSTR("inverseViewMatrix")))
				_members.emplace_back(Feature::InverseViewMatrix, offset);
			else if(name->IsEqual(RNCSTR("inverseViewProjectionMatrix")))
				_members.emplace_back(Feature::InverseViewProjectionMatrix, offset);
			else if(name->IsEqual(RNCSTR("inverseProjectionMatrix")))
				_members.emplace_back(Feature::InverseProjectionMatrix, offset);
			else if(name->IsEqual(RNCSTR("ambientColor")))
				_members.emplace_back(Feature::AmbientColor, offset);
			else if(name->IsEqual(RNCSTR("diffuseColor")))
				_members.emplace_back(Feature::DiffuseColor, offset);
			else if(name->IsEqual(RNCSTR("specularColor")))
				_members.emplace_back(Feature::SpecularColor, offset);
			else if(name->IsEqual(RNCSTR("emissiveColor")))
				_members.emplace_back(Feature::EmissiveColor, offset);
			else if(name->IsEqual(RNCSTR("discardThreshold")))
				_members.emplace_back(Feature::DiscardThreshold, offset);
			else
				_members.emplace_back(name, offset);

			_supportedFeatures |= _members.back().GetFeature();
		}*/
	}

	D3D12UniformBuffer::~D3D12UniformBuffer()
	{
		for(size_t i = 0; i < kRND3D12UniformBufferCount; i ++)
			_buffers[i]->Release();
	}

	GPUBuffer *D3D12UniformBuffer::Advance()
	{
		_bufferIndex = (_bufferIndex + 1) % kRND3D12UniformBufferCount;
		return _buffers[_bufferIndex];

		return nullptr;
	}

	const D3D12UniformBuffer::Member *D3D12UniformBuffer::GetMemberForFeature(Feature feature) const
	{
		/*if(!(_supportedFeatures & feature))
			return nullptr;

		for(const Member &member : _members)
		{
			if(member.GetFeature() == feature)
				return &member;
		}*/
		
		return nullptr;
	}
}

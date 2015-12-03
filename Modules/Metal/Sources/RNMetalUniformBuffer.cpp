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

	MetalUniformBuffer::MetalUniformBuffer(Renderer *renderer, MetalRenderingStateUniformBufferArgument *uniformBuffer) :
		_bufferIndex(0),
		_supportedFeatures(0),
		_index(uniformBuffer->index)
	{
		for(size_t i = 0; i < kRNMetalUniformBufferCount; i ++)
			_buffers[i] = renderer->CreateBufferWithLength(uniformBuffer->size, GPUResource::UsageOptions::WriteOnly);

		AutoreleasePool pool;

		for(MetalRenderingStateUniformBufferMember *member : uniformBuffer->members)
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
		}
	}

	MetalUniformBuffer::~MetalUniformBuffer()
	{
		for(size_t i = 0; i < kRNMetalUniformBufferCount; i ++)
			_buffers[i]->Release();
	}

	GPUBuffer *MetalUniformBuffer::Advance()
	{
		_bufferIndex = (_bufferIndex + 1) % kRNMetalUniformBufferCount;
		return _buffers[_bufferIndex];
	}

	const MetalUniformBuffer::Member *MetalUniformBuffer::GetMemberForFeature(Feature feature) const
	{
		if(!(_supportedFeatures & feature))
			return nullptr;

		for(const Member &member : _members)
		{
			if(member.GetFeature() == feature)
				return &member;
		}

		return nullptr;
	}
}

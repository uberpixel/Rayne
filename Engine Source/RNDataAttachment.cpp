//
//  RNDataAttachment.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNDataAttachment.h"
#include "RNRenderer.h"

namespace RN
{
	DataAttachment::DataAttachment() :
		_buffer(nullptr)
	{}
	
	DataAttachment::~DataAttachment()
	{
		SafeRelease(_buffer);
	}
	
	void DataAttachment::SetBuffer(GPUBuffer *buffer)
	{
		SafeRelease(_buffer);
		_buffer = SafeRetain(buffer);
	}
	
	void DataAttachment::UpdateRenderingObject(RenderingObject &object)
	{
		object.buffers.push_back(_buffer);
	}
}

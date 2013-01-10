//
//  RNRendererFrontend.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRendererFrontend.h"
#include "RNRendererBackend.h"

namespace RN
{
	RendererFrontend::RendererFrontend()
	{
		_committedFrame = _buildFrame = 0;
		_frameNumber = 0;
		
		_frameLock = new Mutex();
		_backend   = new RendererBackend(this);
	}
	
	RendererFrontend::~RendererFrontend()
	{
		_frameLock->Release();
		_backend->Release();
		
		if(_buildFrame)
			delete _buildFrame;
		
		if(_committedFrame)
			delete _committedFrame;
	}
	
	
	
	void RendererFrontend::BeginFrame()
	{
		RN_ASSERT0(_buildFrame == 0);
		_buildFrame = new std::vector<RenderingGroup>();
	}
	
	void RendererFrontend::CommitFrame()
	{
		_frameLock->Lock();
		
		if(!_tookFrame && _committedFrame)
			delete _committedFrame;
		
		_committedFrame = _buildFrame;
		_buildFrame = 0;
		_tookFrame = false;
		
		_frameNumber ++;
		
		_frameLock->Unlock();
	}
	
	void RendererFrontend::PushGroup(const RenderingGroup& group)
	{
		_buildFrame->push_back(group);
	}
	
	
	
	uint32_t RendererFrontend::CommittedFrame(std::vector<RenderingGroup> **frame)
	{
		RN_ASSERT0(frame != 0);
		
		_frameLock->Lock();
		
		uint32 frameID = _frameNumber;
		*frame = _committedFrame;
		
		_tookFrame = true;
		_frameLock->Unlock();
		
		return frameID;
	}
}

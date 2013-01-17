//
//  RNRendererFrontend.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
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
		_backend->Release();
		_frameLock->Release();
		
		if(_buildFrame)
			delete _buildFrame;

		if(!_tookFrame && _committedFrame)
			delete _committedFrame;
	}
	
	
	
	void RendererFrontend::BeginFrame()
	{
		RN_ASSERT0(_buildFrame == 0);
		_buildFrame = new std::vector<RenderingGroup>();
	}
	
	void RendererFrontend::CommitFrame(float time)
	{
		_frameLock->Lock();
		
		if(!_tookFrame && _committedFrame)
			delete _committedFrame;
		
		_committedFrame = _buildFrame;
		_time = time;
		
		_buildFrame = 0;
		_tookFrame = false;
		
		_frameNumber ++;
		
		_frameLock->Unlock();
	}
	
	void RendererFrontend::PushGroup(const RenderingGroup& group)
	{
		_buildFrame->push_back(group);
	}
	
	
	
	uint32_t RendererFrontend::CommittedFrame(std::vector<RenderingGroup> **frame, float *time)
	{
		RN_ASSERT0(frame && time);
		
		_frameLock->Lock();
		
		uint32 frameID = _frameNumber;
		
		*frame = _committedFrame;
		*time = _time;
		
		_tookFrame = true;

		_frameLock->Unlock();
		
		return frameID;
	}
}

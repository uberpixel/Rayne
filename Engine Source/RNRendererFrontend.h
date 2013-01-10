//
//  RNRendererFrontend.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERERFRONTEND_H__
#define __RAYNE_RENDERERFRONTEND_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNRendering.h"
#include "RNMutex.h"

namespace RN
{
	class RendererBackend;
	class RendererFrontend : public Object
	{
	friend class RendererBackend;
	public:
		RendererFrontend();
		virtual ~RendererFrontend();
		
		void BeginFrame();
		void CommitFrame();
		
		void PushGroup(const RenderingGroup& group);
		
		RendererBackend *Backend() { return _backend; }
		
	private:
		uint32_t CommittedFrame(std::vector<RenderingGroup> **frame);
		
		Mutex *_frameLock;
		uint32 _frameNumber;
		bool _tookFrame;
		
		std::vector<RenderingGroup> *_buildFrame;
		std::vector<RenderingGroup> *_committedFrame;
		
		RendererBackend *_backend;
	};
}

#endif /* __RAYNE_RENDERERFRONTEND_H__ */

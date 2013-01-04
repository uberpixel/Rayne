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
#include "RNRenderering.h"
#include "RNMutex.h"

namespace RN
{
	class RendererBackend;
	class RendererFrontend : public Object
	{
	public:
		RendererFrontend();
		virtual ~RendererFrontend();
		
		void BeginFrame();
		void CommitFrame();
		
		uint32_t CommittedFrame(std::vector<RenderingIntent> **frame);
		
		void PushIntent(const RenderingIntent& command);
		
		RendererBackend *Backend() { return _backend; }
		
	private:
		Mutex *_frameLock;
		uint32 _frameNumber;
		bool _tookFrame;
		
		std::vector<RenderingIntent> *_buildFrame;
		std::vector<RenderingIntent> *_committedFrame;
		
		RendererBackend *_backend;
	};
}

#endif /* __RAYNE_RENDERERFRONTEND_H__ */

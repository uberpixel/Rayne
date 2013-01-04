//
//  RNRendererBackend.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERERBACKEND_H__
#define __RAYNE_RENDERERBACKEND_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNRenderering.h"

namespace RN
{
	class RendererFrontend;
	class RendererBackend : public Object
	{
	public:
		RendererBackend(RendererFrontend *frontend);
		virtual ~RendererBackend();
		
		void SetDefaultCamera(Camera *camera);
		Camera *DefaultCamera();
		
		void DrawFrame();
		void PrepareFrame(std::vector<RenderingIntent> *frame);
		
	protected:
		virtual void DrawFrame(std::vector<RenderingIntent> *frame);
		virtual void BindMaterial(Material *material);
		virtual void DrawMesh(Mesh *mesh);
		
		bool _texture2DEnabled;
		bool _cullingEnabled;
		bool _depthTestEnabled;
		bool _blendingEnabled;
		
		GLenum _cullMode;
		GLenum _depthFunc;
		
		GLenum _blendSource;
		GLenum _blendDestination;
		
		Camera *_defaultCamera;
		RendererFrontend *_frontend;
		
		Material *_currentMaterial;
		Mesh *_currentMesh;
		
		SpinLock _drawLock;
		uint32 _lastFrameID;
		std::vector<RenderingIntent> *_lastFrame;
	};
}

#endif /* __RAYNE_RENDERERBACKEND_H__ */

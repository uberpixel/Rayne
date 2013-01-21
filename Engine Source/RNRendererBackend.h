//
//  RNRendererBackend.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERERBACKEND_H__
#define __RAYNE_RENDERERBACKEND_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNRendering.h"

namespace RN
{
	class RendererFrontend;
	class RendererBackend : public Object
	{
	friend class RendererFrontend;
	public:
		RNAPI RendererBackend(RendererFrontend *frontend);
		RNAPI virtual ~RendererBackend();
		
		RNAPI void SetDefaultFBO(GLuint fbo);
		RNAPI void SetDefaultFrame(uint32 width, uint32 height);
		
		RNAPI void DrawFrame();
		RNAPI void PrepareFrame(std::vector<RenderingGroup> *frame);

	private:
		void DrawGroup(RenderingGroup *group);
		void BindMaterial(Material *material);
		void DrawMesh(Mesh *mesh);
		void FlushCamera(Camera *target, Camera *source);
		
		void InitializeFramebufferCopy();
		void FlushCameras();
		
		GLuint VAOForTuple(const std::tuple<Material *, MeshLODStage *>& tuple);
		
		RendererFrontend *_frontend;
		bool _hasValidFramebuffer;

		float _time;
		machine_uint _activeTextureUnits;
		
		bool _cullingEnabled;
		bool _depthTestEnabled;
		bool _blendingEnabled;
		bool _depthWrite;
		
		GLenum _cullMode;
		GLenum _depthFunc;
		
		GLenum _blendSource;
		GLenum _blendDestination;
		
		std::map<std::tuple<Material *, MeshLODStage *>, GLuint> _vaos;
		GLuint _currentVAO;
		
		GLuint _defaultFBO;
		uint32 _defaultWidth;
		uint32 _defaultHeight;
		
		Material *_currentMaterial;
		Mesh *_currentMesh;
		
		SpinLock _drawLock;
		uint32 _lastFrameID;
		std::vector<RenderingGroup> *_lastFrame;
		std::vector<Camera *> _flushCameras;
		
		Shader *_copyShader;
		GLuint _copyVAO;
		GLuint _copyVBO;
		GLuint _copyIBO;
		
		Vector4 _copyVertices[4];
		GLshort _copyIndices[6];
	};
}

#endif /* __RAYNE_RENDERERBACKEND_H__ */

//
//  RNRenderingPipeline.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERINGPIPELINE_H__
#define __RAYNE_RENDERINGPIPELINE_H__

#include "RNBase.h"
#include "RNRendering.h"
#include "RNPipelineSegment.h"

namespace RN
{
	class Window;
	class RenderingPipeline : public PipelineSegment
	{
	friend class Window;
	public:
		RenderingPipeline();
		virtual ~RenderingPipeline();
		
		RNAPI void PushGroup(const RenderingGroup& group);
		RNAPI void FinishFrame();
		RNAPI void PepareFrame();

		RNAPI void SetDefaultFBO(GLuint fbo);
		RNAPI void SetDefaultFrame(uint32 width, uint32 height);
		
	private:
		void InitializeFramebufferCopy();
		GLuint VAOForTuple(const std::tuple<Material *, MeshLODStage *>& tuple);
		
		void BindMaterial(Material *material);
		uint32 BindTexture(Texture *texture);
		
		void DrawGroup(RenderingGroup *group);
		void DrawMesh(Mesh *mesh);
		void DrawCameraStage(Camera *camera, Camera *stage);
		
		void FlushCameras();
		void FlushCamera(Camera *camera);
		
		virtual void WorkOnTask(TaskID task, float delta);
		
		bool _hasValidFramebuffer;
		
		Mutex *_frameLock;
		bool _finishFrame;
		uint32 _pushedGroups;
		std::vector<RenderingGroup> _frame;
		
		float _scaleFactor;
		float _time;
		
		std::map<Texture *, std::tuple<uint32, uint32>> _boundTextures;
		std::vector<bool> _activeTextures;
		
		uint32 _maxTextureUnits;
		
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
		
		std::vector<Camera *> _flushCameras;
		uint32 _textureTag;
		
		Shader *_copyShader;
		GLuint _copyVAO;
		GLuint _copyVBO;
		GLuint _copyIBO;
		
		Vector4 _copyVertices[4];
		GLshort _copyIndices[6];
	};
}

#endif /* __RAYNE_RENDERINGPIPELINE_H__ */

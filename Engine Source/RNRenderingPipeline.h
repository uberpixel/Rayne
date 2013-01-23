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
		
		virtual void WorkOnTask(TaskID task);
		
		RNAPI void PushGroup(const RenderingGroup& group);
		RNAPI void FinishFrame();

		RNAPI void SetDefaultFBO(GLuint fbo);
		RNAPI void SetDefaultFrame(uint32 width, uint32 height);
		
	private:
		void InitializeFramebufferCopy();		
		GLuint VAOForTuple(const std::tuple<Material *, MeshLODStage *>& tuple);
		
		void DrawGroup(RenderingGroup *group);
		void BindMaterial(Material *material);
		void DrawMesh(Mesh *mesh);
		
		void FlushCameras();
		void FlushCamera(Camera *target, Camera *source);
		
		bool _hasValidFramebuffer;
		
		Mutex *_frameLock;
		bool _finishFrame;
		std::vector<RenderingGroup> _frame;
		
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
		
		std::vector<Camera *> _flushCameras;
		
		Shader *_copyShader;
		GLuint _copyVAO;
		GLuint _copyVBO;
		GLuint _copyIBO;
		
		Vector4 _copyVertices[4];
		GLshort _copyIndices[6];
	};
}

#endif /* __RAYNE_RENDERINGPIPELINE_H__ */

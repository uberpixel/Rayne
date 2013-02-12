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
#include "RNEntity.h"

#define kRNRenderingPipelineInstancingCutOff 100

namespace RN
{
	struct RenderingObject
	{
		const Mesh *mesh;
		const Material *material;
		const Matrix *transform;
	};

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
		void Initialize();
		GLuint VAOForTuple(const std::tuple<Material *, MeshLODStage *>& tuple);

		void BindMaterial(Material *material);
		uint32 BindTexture(Texture *texture);

		void DrawGroup(RenderingGroup *group);
		void DrawMesh(Mesh *mesh);
		void DrawMeshInstanced(Material *material, std::vector<RenderingObject>::iterator begin, const std::vector<RenderingObject>::iterator& last, uint32 count);
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

		uint32 _textureUnit;
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

		Matrix *_instancingMatrices;
		uint32 _numInstancingMatrices;
		GLuint _instancingVBO;

		GLuint _defaultFBO;
		uint32 _defaultWidth;
		uint32 _defaultHeight;

		Camera *_currentCamera;
		Material *_currentMaterial;
		Mesh *_currentMesh;

		std::vector<Camera *> _flushCameras;

		Shader *_copyShader;
		GLuint _copyVAO;
		GLuint _copyVBO;
		GLuint _copyIBO;

		Vector4 _copyVertices[4];
		GLshort _copyIndices[6];

		GLuint _lightTextures[4];
		GLuint _lightBuffers[4];
		
		uint32 _lightBufferLengths[3];
	};
}

#endif /* __RAYNE_RENDERINGPIPELINE_H__ */

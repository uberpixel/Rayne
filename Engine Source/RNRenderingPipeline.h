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

namespace RN
{
	class Skeleton;
	struct RenderingObject
	{
		const Mesh *mesh;
		const Material *material;
		const Matrix *transform;
		const Skeleton *skeleton;
	};

	class Window;
	class RenderingPipeline : public PipelineSegment
	{
	friend class Window;
	public:
		RenderingPipeline();
		virtual ~RenderingPipeline();

		RNAPI void PushGroup(RenderingGroup *group);
		RNAPI void FinishFrame();
		RNAPI void PepareFrame();

		RNAPI void SetDefaultFBO(GLuint fbo);
		RNAPI void SetDefaultFrame(uint32 width, uint32 height);

	private:
		void Initialize();
		
		void BindMaterial(Material *material, ShaderProgram *program);
		void BindShader(ShaderProgram *shader);
		uint32 BindTexture(Texture *texture);
		GLuint BindVAO(const std::tuple<ShaderProgram *, MeshLODStage *>& tuple);
		
		void UpdateShaderWithCamera(ShaderProgram *shader, Camera *camera);
		void CreateLightList(RenderingGroup *group, Camera *camera, Vector4 **outLightPos, Vector3 **outLightColor, int *outLightCount);

		void DrawGroup(RenderingGroup *group);
		void DrawMesh(Mesh *mesh, ShaderProgram *program);
		void DrawMeshInstanced(const Array<RenderingObject>& group, machine_uint start, machine_uint count);
		void DrawCameraStage(Camera *camera, Camera *stage);

		void FlushCameras();
		void FlushCamera(Camera *camera);

		virtual void WorkOnTask(TaskID task, float delta);

		bool _initialized;
		bool _hasValidFramebuffer;

		Mutex *_frameLock;
		bool _finishFrame;
		uint32 _pushedGroups;
		std::vector<RenderingGroup *> _frame;

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
		
		int *_lightindexoffset;
		int *_lightindices;
		size_t _lightindexoffsetSize;
		size_t _lightindicesSize;

		std::map<std::tuple<ShaderProgram *, MeshLODStage *>, GLuint> _vaos;
		
		Matrix *_instancingMatrices;
		uint32 _numInstancingMatrices;
		GLuint _instancingVBO;

		GLuint _defaultFBO;
		uint32 _defaultWidth;
		uint32 _defaultHeight;

		Camera   *_currentCamera;
		Material *_currentMaterial;
		GLuint _currentShader;
		GLuint _currentVAO;
		
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

//
//  RNCamera.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CAMERA_H__
#define __RAYNE_CAMERA_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNRenderingResource.h"
#include "RNRect.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNColor.h"
#include "RNTexture.h"
#include "RNMaterial.h"

namespace RN
{
	class Camera : public Object, public RenderingResource
	{
	public:
		enum
		{
			FlagDrawTarget = (1 << 0),
			FlagUpdateAspect = (1 << 1),
			FlagInherit = (1 << 2)
		};
		typedef uint32 Flags;
		
		RNAPI Camera(const Vector2& size, Flags flags=FlagUpdateAspect);
		RNAPI virtual ~Camera();
		
		RNAPI void Bind();
		RNAPI void Unbind();
		RNAPI virtual void PrepareForRendering();
		
		RNAPI void CreateBuffer(bool depthbuffer, bool stencilbuffer);
		
		RNAPI virtual void SetFrame(const Rect& frame);
		RNAPI void SetClearColor(const Color& color);
		RNAPI void SetMaterial(Material *material);
		
		RNAPI void AddStage(Camera *stage);
		RNAPI void InsertStage(Camera *stage);
		RNAPI void RemoveStage(Camera *stage);
		
		RNAPI virtual void UpdateProjection();
		RNAPI virtual void UpdateCamera();
		
		const Rect& Frame() const { return _frame; }
		const Color& ClearColor() const { return _clearColor; }
		const bool& Linear() const { return _isLinear; }
		
		const Matrix& ProjectionMatrix() const { return _projectionMatrix; }
		const Matrix& InverseProjectionMatrix() const { return _inverseProjectionMatrix; }
		
		const Matrix& ViewMatrix() const { return _viewMatrix; }
		const Matrix& InverseViewMatrix() const { return _inverseViewMatrix; }
		
		GLuint Framebuffer() const { return _framebuffer; }
		GLuint Depthbuffer() const { return _depthbuffer; }
		GLuint Stencilbuffer() const { return _stencilbuffer; }
		
		Texture *Target() const { return _texture; }
		Material *Material() const { return _material; }
		Camera *Stage() const { return _stage; }
		
		float arc;
		float aspect;
		float clipnear;
		float clipfar;
		
		Flags flags;
		
		Vector3 position;
		Quaternion rotation;
		
	protected:
		void CheckError();
		
		Rect _frame;
		Color _clearColor;
		bool _isLinear;
		
		Matrix _projectionMatrix;
		Matrix _inverseProjectionMatrix;
		
		Matrix _viewMatrix;
		Matrix _inverseViewMatrix;
		
		GLuint _framebuffer;
		GLuint _depthbuffer;
		GLuint _stencilbuffer;
		
	private:
		void UpdateStage(bool updateFrame) const;
		
		bool _packedStencil;
		int _current;
		
		Texture *_texture;
		Camera *_stage;
		class Material *_material;
		
		void SetDefaultValues();
	};
}

#endif /* __RAYNE_CAMERA_H__ */

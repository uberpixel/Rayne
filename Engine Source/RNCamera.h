//
//  RNCamera.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
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
		
		Camera(const Vector2& size, Flags flags=FlagUpdateAspect);
		virtual ~Camera();
		
		void Bind();
		void Unbind();
		virtual void PrepareForRendering();
		
		virtual void SetFrame(const Rect& frame);
		void SetClearColor(const Color& color);
		void SetMaterial(Material *material);
		
		void AddStage(Camera *stage);
		void InsertStage(Camera *stage);
		void RemoveStage(Camera *stage);
		
		virtual void UpdateProjection();
		virtual void UpdateCamera();
		
		const Rect& Frame() const { return _frame; }
		const Color& ClearColor() const { return _clearColor; }
		
		const Matrix& ProjectionMatrix() const { return _projectionMatrix; }
		const Matrix& InverseProjectionMatrix() const { return _inverseProjectionMatrix; }
		
		const Matrix& ViewMatrix() const { return _viewMatrix; }
		const Matrix& InverseViewMatrix() const { return _inverseViewMatrix; }
		
		GLuint DepthBuffer() const { return _depthBuffer; }
		GLuint StencilBuffer() const { return _stencilBuffer; }
		
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
		
		Matrix _projectionMatrix;
		Matrix _inverseProjectionMatrix;
		
		Matrix _viewMatrix;
		Matrix _inverseViewMatrix;
		
		GLuint _frameBuffer;
		GLuint _depthBuffer;
		GLuint _stencilBuffer;
		
	private:
		int _current;
		
		Texture *_texture;
		Camera *_stage;
		class Material *_material;
		
		void SetDefaultValues();
	};
}

#endif /* __RAYNE_CAMERA_H__ */

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
#include "RNRect.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNColor.h"

namespace RN
{
	class Camera : public Object
	{
	public:
		Camera(const Vector2& size);
		virtual ~Camera();
		
		void Bind();
		void Unbind();
		virtual void PrepareForRendering();
		
		virtual void SetFrame(const Rect& frame);
		void SetClearColor(const Color& color);
		
		virtual void UpdateProjection();
		virtual void UpdateCamera();
		
		const Rect& Frame() const;
		const Color& ClearColor() const;
		
		const Matrix& ProjectionMatrix() const { return _projectionMatrix; }
		const Matrix& InverseProjectionMatrix() const { return _inverseProjectionMatrix; }
		
		const Matrix& ViewMatrix() const { return _viewMatrix; }
		const Matrix& InverseViewMatrix() const { return _inverseViewMatrix; }
		
		float arc;
		float aspect;
		float clipnear;
		float clipfar;
		
		Vector3 position;
		Quaternion rotation;
		
	protected:
		Rect _frame;
		Color _clearColor;
		
		Matrix _projectionMatrix;
		Matrix _inverseProjectionMatrix;
		
		Matrix _viewMatrix;
		Matrix _inverseViewMatrix;
		
		GLuint _frameBuffer;
		GLuint _colorBuffer;
		GLuint _depthBuffer;
		GLuint _stencilBuffer;
		
	private:
		bool _bound;
		Camera *_previous;
	};
}

#endif /* __RAYNE_CAMERA_H__ */

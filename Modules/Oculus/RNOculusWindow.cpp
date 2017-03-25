//
//  RNOculusWindow.cpp
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusSwapChain.h"
#include "RNOculusWindow.h"

namespace RN
{
	RNDefineMeta(OculusWindow, Window)

	OculusWindow::OculusWindow()
	{
		_swapChain = new OculusSwapChain();
	}

	OculusWindow::~OculusWindow()
	{
		_swapChain->Release();
	}

	Vector2 OculusWindow::GetSize() const
	{
		return _swapChain->GetSize();
	}

	Framebuffer *OculusWindow::GetFramebuffer() const
	{
		return _swapChain->GetFramebuffer();
	}

	void OculusWindow::UpdateTrackingData()
	{
		_swapChain->UpdatePredictedPose();
	}

	Vector3 OculusWindow::GetHeadPosition() const
	{
		ovrVector3f position = _swapChain->_hmdState.HeadPose.ThePose.Position;
		return Vector3(position.x, position.y, position.z);
	}

	Quaternion OculusWindow::GetHeadRotation() const
	{
		ovrQuatf rotation = _swapChain->_hmdState.HeadPose.ThePose.Orientation;
		return Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
	}

	Matrix OculusWindow::GetProjectionMatrix(int eye, float near, float far) const
	{
		ovrMatrix4f proj = ovrMatrix4f_Projection(_swapChain->_layer.Fov[eye], near, far, ovrProjection_None);
		Matrix result;

		for(int q = 0; q < 4; q++)
		{
			for(int t = 0; t < 4; t++)
			{
				result.m[q*4 + t] = proj.M[t][q];
			}
		}

		return result;
	}

	Vector3 OculusWindow::GetEyePosition(int eye) const
	{
		// Get view and projection matrices for the Rift camera
		/*		Vector3f pos = originPos + originRot.Transform(layer.RenderPose[eye].Position);
		Matrix4f rot = originRot * Matrix4f(layer.RenderPose[eye].Orientation);
		Vector3f finalUp = rot.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = rot.Transform(Vector3f(0, 0, -1));
		Matrix4f view = Matrix4f::LookAtRH(pos, pos + finalForward, finalUp);
		Matrix4f proj = ovrMatrix4f_Projection(layer.Fov[eye], 0.2f, 1000.0f, 0);*/

		ovrVector3f position = _swapChain->_layer.RenderPose[eye].Position;
		return Vector3(position.x, position.y, position.z);
	}

	Quaternion OculusWindow::GetEyeRotation(int eye) const
	{
		ovrQuatf rotation = _swapChain->_layer.RenderPose[eye].Orientation;
		return Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
	}
}

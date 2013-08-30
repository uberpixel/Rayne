//
//  ROVRCamera.cpp
//  rayne-oculus
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "ROVRCamera.h"

namespace RN
{
	namespace oculus
	{
		Camera::Camera()
		{
			System *system = System::GetSharedInstance();
			
			_eyecenteroffset = RN::Vector3(0.0f, 0.12f, -0.04f);
			
			float eyedistance = 0.064;
			float screenwidth = 0.14976;
			float screenheight = 0.0936;
			float screendist = 0.041;
			float lensdist = 0.0635;
			RN::Vector4 hmdwarpparam(1.0f, 0.22f, 0.24f, 0.0f);
			RN::Vector4 chromabparam(0.996f, -0.004f, 1.014, 0.0f);
			
			if(system->GetHMDConnected())
			{
				HMDInfo info = system->GetHMDInfo();
				eyedistance = info.InterpupillaryDistance;
				screenwidth = info.HScreenSize;
				screenheight = info.VScreenSize;
				screendist = info.EyeToScreenDistance;
				lensdist = info.LensSeparationDistance;
				
				hmdwarpparam.x = info.DistortionK[0];
				hmdwarpparam.y = info.DistortionK[1];
				hmdwarpparam.z = info.DistortionK[2];
				hmdwarpparam.w = info.DistortionK[3];
				
				chromabparam.x = info.ChromaAbCorrection[0];
				chromabparam.y = info.ChromaAbCorrection[1];
				chromabparam.z = info.ChromaAbCorrection[2];
				chromabparam.w = info.ChromaAbCorrection[3];
			}
			
			_projshift = 1.0f-2.0f*lensdist/screenwidth;
			_eyeshift = eyedistance*0.5f;
			
			RN::Vector2 left_lenscenter = RN::Vector2(0.25f+_projshift*0.25f, 0.5f);
			RN::Vector2 left_screencenter = RN::Vector2(0.25f, 0.5f);
			
			RN::Vector2 right_lenscenter = RN::Vector2(0.75f-_projshift*0.25f, 0.5f);
			RN::Vector2 right_screencenter = RN::Vector2(0.75f, 0.5f);
			
			float lensradius = -1.0f-_projshift;
			float lensradsq = lensradius*lensradius;
			_scalefac = hmdwarpparam.x+hmdwarpparam.y*lensradsq+hmdwarpparam.z*lensradsq*lensradsq+hmdwarpparam.w*lensradsq*lensradsq*lensradsq;
			//_scalefac *= 0.3f;
			
			float aspect = screenwidth*0.5f/screenheight;
			_riftfov = 2.0f*atan(screenheight*_scalefac/(2.0f*screendist))*180.0f/RN::k::Pi;
			
			RN::Vector2 scale = RN::Vector2(0.25f, 0.5f*aspect)/_scalefac;
			RN::Vector2 scalein = RN::Vector2(4.0f, 2.0f/aspect);
			
			
			RN::Shader *riftshader = RN::Shader::WithFile("assets/shader/RiftDistortion");
			
			_storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatComplete);
			_storage->AddRenderTarget(RN::TextureParameter::Format::RGBA32F);
			
			
			_left = new RN::Camera(RN::Vector2(), _storage, RN::Camera::FlagUpdateAspect);
			_left->fov = _riftfov;
			_left->clipnear = 0.01f;
			_left->clipfar = 200.0f;
			_left->usefog = false;
			_left->UpdateProjection();
			_left->SetBlitMode(RN::Camera::BlitMode::Unstretched);
			_left->SetPriority(100);
			_left->SetDrawFramebufferShader(RN::Shader::WithFile("shader/rn_DrawFramebufferTonemap"));
			_left->SetDebugName("Left");
			_left->SetClearColor(RN::Color::White()*5.0f);
			
			RN::PostProcessingPipeline *ppp_left = _left->AddPostProcessingPipeline("Rift left");
			RN::Material *left_riftmat = new RN::Material(riftshader);
			
			_left_riftstage = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
			_left_riftstage->SetBlitMode(RN::Camera::BlitMode::Unstretched);
			_left_riftstage->SetMaterial(left_riftmat);
			_left_riftstage->SetDebugName("Left Riftstage");
			_left_riftstage->SetClearColor(RN::Color::White()*5.0f);
			
			ppp_left->AddStage(_left_riftstage, RN::RenderStage::Mode::ReUsePreviousStage);
			
			left_riftmat->AddShaderUniform("LensCenter", left_lenscenter);
			left_riftmat->AddShaderUniform("ScreenCenter", left_screencenter);
			left_riftmat->AddShaderUniform("Scale", scale);
			left_riftmat->AddShaderUniform("ScaleIn", scalein);
			left_riftmat->AddShaderUniform("HmdWarpParam", hmdwarpparam);
			left_riftmat->AddShaderUniform("ChromAbParam", chromabparam);
			
			
			_right = new RN::Camera(RN::Vector2(), _storage, RN::Camera::FlagUpdateAspect);
			_right->fov = _riftfov;
			_right->clipnear = 0.01f;
			_right->clipfar = 200.0f;
			_right->usefog = false;
			_right->UpdateProjection();
			_right->SetBlitMode(RN::Camera::BlitMode::Unstretched);
			_right->SetPriority(100);
			_right->SetDrawFramebufferShader(RN::Shader::WithFile("shader/rn_DrawFramebufferTonemap"));
			_right->SetDebugName("Right");
			_right->SetClearColor(RN::Color::White()*5.0f);
			
			RN::PostProcessingPipeline *ppp_right = _right->AddPostProcessingPipeline("Rift right");
			RN::Material *right_riftmat = new RN::Material(riftshader);
			
			_right_riftstage = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
			_right_riftstage->SetBlitMode(RN::Camera::BlitMode::Unstretched);
			_right_riftstage->SetMaterial(right_riftmat);
			_right_riftstage->SetDebugName("Right riftstage");
			_right_riftstage->SetClearColor(RN::Color::White()*5.0f);
			
			ppp_right->AddStage(_right_riftstage, RN::RenderStage::Mode::ReUsePreviousStage);
			
			
			right_riftmat->AddShaderUniform("LensCenter", right_lenscenter);
			right_riftmat->AddShaderUniform("ScreenCenter", right_screencenter);
			right_riftmat->AddShaderUniform("Scale", scale);
			right_riftmat->AddShaderUniform("ScaleIn", scalein);
			right_riftmat->AddShaderUniform("HmdWarpParam", hmdwarpparam);
			right_riftmat->AddShaderUniform("ChromAbParam", chromabparam);
			
			
			RN::MessageCenter::GetSharedInstance()->AddObserver(kRNWindowConfigurationChanged, &RN::oculus::Camera::WindowFrameChanged, this, this);
			WindowFrameChanged(nullptr);
		}
		
		Camera::~Camera()
		{
			RN::MessageCenter::GetSharedInstance()->RemoveObserver(this);
			_storage->Release();
		}
		
		void Camera::WindowFrameChanged(RN::Message *message)
		{
			RN::Rect frame = RN::Window::GetSharedInstance()->GetFrame();
			
			_left_riftstage->SetFrame(RN::Rect(0.0f, 0.0f, frame.width*0.5f, frame.height));
			_right_riftstage->SetFrame(RN::Rect(frame.width*0.5f, 0.0f, frame.width*0.5f, frame.height));
			
			_left_riftstage->SetRenderingFrame(RN::Rect(0.0f, 0.0f, frame.width*0.5f, frame.height));
			_right_riftstage->SetRenderingFrame(RN::Rect(0.0f, 0.0f, frame.width*0.5f, frame.height));
			
			frame.width *= _scalefac;
			frame.height *= _scalefac;
			
			_storage->SetSize(frame.Size());
			
			_left->SetFrame(RN::Rect(0.0, 0.0, frame.width * 0.5f, frame.height));
			_right->SetFrame(RN::Rect(frame.width * 0.5f, 0.0, frame.width * 0.5f, frame.height));
			
			
			RN::Matrix leftmat;
			leftmat.MakeTranslate(RN::Vector3(_projshift, 0.0f, 0.0f));
			_left->projectionMatrix = leftmat*_left->projectionMatrix;
			RN::Matrix rightmat;
			rightmat.MakeTranslate(RN::Vector3(-_projshift, 0.0f, 0.0f));
			_right->projectionMatrix = rightmat*_right->projectionMatrix;
		}
		
		void Camera::Update(float delta)
		{
			System *system = System::GetSharedInstance();
			if(system->GetHMDConnected())
			{
				HMDSensors sensors = system->GetHMDSensors();
				_headrotation = sensors.orientation;
			}
			
			RN::Quaternion finalrot = GetWorldRotation()*_headrotation;
			RN::Vector3 finalpos = GetWorldPosition()+finalrot.RotateVector(_eyecenteroffset);
			
			_left->SetRotation(finalrot);
			RN::Vector3 camdiff = -_left->Right()*_eyeshift;
			_left->SetPosition(finalpos+camdiff);
			_right->SetRotation(finalrot);
			_right->SetPosition(finalpos-camdiff);
		}
	}
}
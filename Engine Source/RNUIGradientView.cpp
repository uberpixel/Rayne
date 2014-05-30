//
//  RNUIGradientView.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIGradientView.h"
#include "RNResourceCoordinator.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(GradientView, View)
	
		GradientView::GradientView() :
			_angle(0.0f),
			_isDirty(true),
			_mesh(nullptr)
		{
			_material = GetBasicMaterial(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(RNCSTR("shader/rn_UIGradientView"), nullptr));
			_material->Retain();
			
			_angleUniform = _material->AddShaderUniform("angle", _angle);
			
			SetStartColor(RN::Color::Black());
			SetEndColor(RN::Color::White());
		}
		
		void GradientView::SetFrame(const Rect &frame)
		{
			View::SetFrame(frame);
			_isDirty = true;
		}
		
		
		void GradientView::SetStartColor(const RN::Color &color)
		{
			_material->SetDiffuseColor(color);
		}
		
		void GradientView::SetEndColor(const RN::Color &color)
		{
			_material->SetAmbientColor(color);
		}
		
		void GradientView::SetAngle(float angle)
		{
			_angleUniform->SetFloatValue(Math::DegreesToRadians(angle));
			_angle = angle;
		}
		
		
		
		void GradientView::Update()
		{
			View::Update();
			
			if(_isDirty)
			{
				if(_mesh)
					UpdateBasicMesh(_mesh, GetFrame().GetSize());
				else
					_mesh = GetBasicMesh(GetFrame().GetSize())->Retain();
				
				_isDirty = false;
			}
		}
		
		void GradientView::Draw(Renderer *renderer)
		{
			View::Draw(renderer);
			
			if(_mesh)
			{
				RenderingObject object;
				PopulateRenderingObject(object);
				
				object.mesh     = _mesh;
				object.material = _material;
				
				renderer->RenderObject(object);
			}
		}
	}
}

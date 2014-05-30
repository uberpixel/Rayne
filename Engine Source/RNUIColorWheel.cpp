//
//  RNUIColorWheel.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIColorWheel.h"
#include "RNResourceCoordinator.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(ColorWheel, View)
		
		ColorWheel::ColorWheel()
		{
			Initialize();
		}
		
		void ColorWheel::Initialize()
		{
			_isDirty = true;
			
			_mesh  = nullptr;
			
			_material = GetBasicMaterial(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(RNCSTR("shader/rn_UIColorWheel"), nullptr));
			_material->Retain();
			
			SetBrightness(1.0f);
		}
		
		void ColorWheel::SetBrightness(float brightness)
		{
			brightness = std::max(0.0f, std::min(1.0f, brightness));
			_material->SetDiffuseColor(RN::Color(0.0f, 0.0f, 0.0f, brightness));
		}
		
		void ColorWheel::Update()
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
		
		void ColorWheel::Draw(Renderer *renderer)
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

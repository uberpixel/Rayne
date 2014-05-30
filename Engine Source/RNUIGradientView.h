//
//  RNUIGradientView.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIGRADIENTVIEW_H__
#define __RAYNE_UIGRADIENTVIEW_H__

#include "RNBase.h"
#include "RNUIView.h"
#include "RNUIColor.h"

namespace RN
{
	namespace UI
	{
		class GradientView : public View
		{
		public:
			GradientView();
			
			void SetFrame(const Rect &frame) override;
			
			void SetStartColor(const RN::Color &color);
			void SetEndColor(const RN::Color &color);
			void SetAngle(float angle);
			
			void Update() override;
			void Draw(Renderer *renderer) override;
			
		private:
			Material *_material;
			Material::ShaderUniform *_angleUniform;
			Mesh *_mesh;
			
			bool _isDirty;
			float _angle;
			
			RNDeclareMeta(GradientView)
		};
	}
}

#endif /* __RAYNE_UIGRADIENTVIEW_H__ */

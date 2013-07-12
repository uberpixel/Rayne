//
//  RNUIImageView.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIIMAGEVIEW_H__
#define __RAYNE_UIIMAGEVIEW_H__

#include "RNBase.h"
#include "RNUIView.h"
#include "RNUIImage.h"

namespace RN
{
	namespace UI
	{
		class ImageView : public View
		{
		public:
			ImageView();
			ImageView(const Rect& frame);
			ImageView(Image *image);
			~ImageView() override;
			
			void SetImage(Image *image);
			void SetScaleMode(ScaleMode mode);
			void SetFrame(const Rect& frame) override;
			
			Vector2 SizeThatFits() override;
			
		protected:
			void Update() override;
			bool Render(RenderingObject& object) override;
			
		private:
			void Initialize();
			Vector2 FittingImageSize(const Vector2& tsize);
			
			bool _isDirty;
			
			ScaleMode _scaleMode;
			Image *_image;
			Mesh  *_mesh;
			
			RNDefineMeta(ImageView, View)
		};
	}
}

#endif /* __RAYNE_UIIMAGEVIEW_H__ */

//
//  RNUIImageView.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
			RNAPI ImageView();
			RNAPI ImageView(const Rect &frame);
			RNAPI ImageView(Image *image);
			RNAPI ~ImageView() override;
			
			RNAPI void SetImage(Image *image);
			RNAPI void SetScaleMode(ScaleMode mode);
			RNAPI void SetFrame(const Rect &frame) override;
			
			RNAPI Image *GetImage() const { return _image; }
			RNAPI ScaleMode GetScaleMode() const { return _scaleMode; }
			
			RNAPI Vector2 GetSizeThatFits() override;
			
		protected:
			RNAPI void Update() override;
			RNAPI void Draw(Renderer *renderer) override;
			
		private:
			void Initialize();
			Vector2 GetFittingImageSize(const Vector2 &tsize);
			
			bool _isDirty;
			
			ScaleMode _scaleMode;
			Image *_image;
			
			Material *_material;
			Mesh  *_mesh;
			
			RNDeclareMeta(ImageView)
		};
	}
}

#endif /* __RAYNE_UIIMAGEVIEW_H__ */

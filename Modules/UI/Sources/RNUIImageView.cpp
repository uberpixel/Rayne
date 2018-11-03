//
//  RNUIImageView.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIImageView.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(ImageView, View)

		ImageView::ImageView() :
			_image(nullptr)
		{}
		ImageView::~ImageView()
		{
			SafeRelease(_image);
		}

		void ImageView::SetImage(Image *image)
		{
			SafeRelease(_image);
			_image = SafeRetain(image);
		}

		void ImageView::Draw(Context *context) const
		{
			View::Draw(context);

			if(_image)
			{
				context->DrawImage(_image, GetBounds());
			}
		}
	}
}

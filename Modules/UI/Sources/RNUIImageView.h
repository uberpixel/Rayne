//
//  RNUIImageView.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIIMAGEVIEW_H_
#define __RAYNE_UIIMAGEVIEW_H_

#include "RNUIView.h"
#include "RNUIImage.h"

namespace RN
{
	namespace UI
	{
		class ImageView : public View
		{
		public:
			UIAPI ImageView();
			UIAPI ~ImageView();

			UIAPI void SetImage(Image *image);

			UIAPI void Draw(Context *context) const override;

		private:
			Image *_image;

			RNDeclareMetaAPI(ImageView, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIIMAGEVIEW_H_ */

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

namespace RN
{
	namespace UI
	{
		class ImageView : public View
		{
		public:
			UIAPI ImageView();
			UIAPI ~ImageView();

			UIAPI void SetImage(Texture *image);
			Texture *GetImage() const { return _image; }
			
		protected:
			UIAPI virtual void UpdateModel() override;

		private:
			Texture *_image;

			RNDeclareMetaAPI(ImageView, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIIMAGEVIEW_H_ */

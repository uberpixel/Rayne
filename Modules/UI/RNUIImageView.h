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
			UIAPI ImageView(Texture *image);
			UIAPI ~ImageView();

			UIAPI void SetImage(Texture *image);
			Texture *GetImage() const { return _image; }
			
			UIAPI void SetColor(Color color); //Multiplicative, including alpha!
			
		protected:
			UIAPI void UpdateModel() override;
			UIAPI void SetOpacityFromParent(float parentCombinedOpacity) override;

		private:
			Texture *_image;
			Color _color;

			RNDeclareMetaAPI(ImageView, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIIMAGEVIEW_H_ */

//
//  RNImageView.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_IMAGEVIEW_H__
#define __RAYNE_IMAGEVIEW_H__

#include "RNBase.h"
#include "RNView.h"
#include "RNImage.h"

namespace RN
{
	class ImageView : public View
	{
	public:
		ImageView();
		ImageView(const Rect& frame);
		ImageView(Image *image);
		~ImageView() override;
		
		void SetImage(Image *image);
		
	protected:
		bool Render(RenderingObject& object) override;
		
	private:
		void Initialize();
		
		Image *_image;
		Mesh *_mesh;
	};
}

#endif /* __RAYNE_IMAGEVIEW_H__ */

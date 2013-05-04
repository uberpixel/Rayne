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
#include "RNTexture.h"
#include "RNMaterial.h"

namespace RN
{
	class ImageView : public View
	{
	public:
		
	private:
		Material *_material;
		Texture *_image;
	};
}

#endif /* __RAYNE_IMAGEVIEW_H__ */

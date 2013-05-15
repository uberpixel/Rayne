//
//  RNImage.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_IMAGE_H__
#define __RAYNE_IMAGE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNTexture.h"

namespace RN
{
	class Image : public Object
	{
	public:
		Image(Texture *texture);
		Image(const std::string& file);
		~Image() override;
		
		static Image *WithFile(const std::string& file);
		
		Texture *Texture() const { return _texture; }
		
		uint32 Width() const { return _texture->Width(); }
		uint32 Height() const { return _texture->Height(); }
		
	private:
		class Texture *_texture;
		
		RNDefineConstructorlessMeta(Image, Object)
	};
}

#endif /* __RAYNE_IMAGE_H__ */

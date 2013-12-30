//
//  RNUIImage.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIIMAGE_H__
#define __RAYNE_UIIMAGE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNTexture.h"
#include "RNMesh.h"
#include "RNUIGeometry.h"

namespace RN
{
	namespace UI
	{
		class Image : public Object
		{
		public:
			RNAPI Image(Texture *texture);
			RNAPI Image(const std::string& file);
			RNAPI ~Image() override;
			
			RNAPI static Image *WithFile(const std::string& file);
			RNAPI static Image *WithTexture(Texture *texture);
			
			RNAPI void SetAtlas(const Atlas& atlas, bool normalized=true);
			RNAPI void SetEdgeInsets(const EdgeInsets& insets);
			
			RNAPI const Atlas& GetAtlas() const { return _atlas; }
			RNAPI const EdgeInsets& GetEdgeInsets() const { return _insets; }
			
			RNAPI Texture *GetTexture() const { return _texture; }
			RNAPI Mesh *GetFittingMesh(const Vector2& size, const Vector2& offset=Vector2());
			
			RNAPI size_t GetWidth() const;
			RNAPI size_t GetHeight() const;
			
		private:
			Texture *_texture;
			Atlas _atlas;
			EdgeInsets _insets;
			
			RNDefineMeta(Image, Object)
		};
	}
}

#endif /* __RAYNE_UIIMAGE_H__ */

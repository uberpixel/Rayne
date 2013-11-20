//
//  RNUIImage.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
			Image(Texture *texture);
			Image(const std::string& file);
			~Image() override;
			
			static Image *WithFile(const std::string& file);
			static Image *WithTexture(Texture *texture);
			
			void SetAtlas(const Atlas& atlas, bool normalized=true);
			void SetEdgeInsets(const EdgeInsets& insets);
			
			const Atlas& GetAtlas() const { return _atlas; }
			const EdgeInsets& GetEdgeInsets() const { return _insets; }
			
			Texture *GetTexture() const { return _texture; }
			Mesh *GetFittingMesh(const Vector2& size, const Vector2& offset=Vector2());
			
			size_t GetWidth() const;
			size_t GetHeight() const;
			
		private:
			Texture *_texture;
			Atlas _atlas;
			EdgeInsets _insets;
			
			RNDefineMeta(Image, Object)
		};
	}
}

#endif /* __RAYNE_UIIMAGE_H__ */

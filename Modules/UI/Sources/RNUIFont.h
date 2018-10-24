//
//  RNUIFont.h
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIFONT_H_
#define __RAYNE_UIFONT_H_

#include "RNUIConfig.h"

namespace RN
{
	namespace UI
	{
		struct FontInternals;
		
		class Font : public Object
		{
		public:
			friend class Context;
			friend class Label;
			
			UIAPI ~Font();
			float GetSize() const { return _size; };
			
			static Font *WithFamilyName(const String *familyName, float size);
			
		private:
			UIAPI Font();
			
			PIMPL<FontInternals> _internals;
			float _size;
			
			RNDeclareMetaAPI(Font, UIAPI)
		};
	}
}

#endif /* __RAYNE_UIFONT_H_ */

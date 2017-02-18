//
//  RNUIImage.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIIMAGE_H_
#define __RAYNE_UIIMAGE_H_

#include "RNUIConfig.h"

namespace RN
{
	namespace UI
	{
		struct ImageInternals;

		class Image : public Object
		{
		public:
			friend class Context;

			UIAPI Image(const Bitmap *bitmap);
			UIAPI static Image *WithName(const String *name);

		private:
			PIMPL<ImageInternals> _internals;

			RNDeclareMetaAPI(Image, UIAPI)
		};
	}
}

#endif /* __RAYNE_UIIMAGE_H_ */

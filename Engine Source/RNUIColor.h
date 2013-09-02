//
//  RNUIColor.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UICOLOR_H__
#define __RAYNE_UICOLOR_H__

#include "RNBase.h"
#include "RNColor.h"
#include "RNObject.h"

namespace RN
{
	namespace UI
	{
		class Color : public Object
		{
		public:
			Color();
			Color(const RN::Color& color);
			Color(Color *other);
			
			static Color *WithRNColor(const RN::Color& color);
			
			bool IsEqual(Object *other) const override;
			
			const RN::Color& GetRNColor() const { return _color; }
			
		private:
			RN::Color _color;
			
			RNDefineMetaWithTraits(Color, Object, MetaClassTraitCopyable);
		};
	}
}

#endif /* __RAYNE_UICOLOR_H__ */

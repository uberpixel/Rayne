//
//  RNUIInternals.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIINTERNALS_H_
#define __RAYNE_UIINTERNALS_H_

#include <skia.h>
#include "RNUIConfig.h"

namespace RN
{
	namespace UI
	{
		struct ContextInternals
		{
			sk_sp<SkSurface> surface;
			SkPaint strokeStyle;
			SkPaint fillStyle;
			std::vector<std::pair<SkPaint, SkPaint>> restoreStyles;
			std::vector<char> backingSurface;
		};

		struct PathInternals
		{
			SkPath path;
		};

		struct ImageInternals
		{
			sk_sp<SkImage> image;
		};



		static SkColor MakeColor(const Color &color)
		{
			SkColor4f intermediate;
			intermediate.fR = color.r;
			intermediate.fG = color.g;
			intermediate.fB = color.b;
			intermediate.fA = color.a;

			return intermediate.toSkColor();
		}
		static SkRect MakeRect(const Rect &rect)
		{
			return SkRect::MakeXYWH(rect.x, rect.y, rect.width, rect.height);
		}
	}
}


#endif /* __RAYNE_UIINTERNALS_H_ */

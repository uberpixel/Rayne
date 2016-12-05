//
//  RNUIPath.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIPATH_H_
#define __RAYNE_UIPATH_H_

#include "RNUIConfig.h"

namespace RN
{
	namespace UI
	{
		struct PathInternals;

		class Path : public Object
		{
		public:
			friend class Context;

			UIAPI Path();
			UIAPI ~Path();

			UIAPI static Path *WithRect(const Rect &rect);
			UIAPI static Path *WithRoundedRect(const Rect &rect, float radius);
			UIAPI static Path *WithRoundedRect(const Rect &rect, float radiusX, float radiusY);

			UIAPI void MoveToPoint(const Vector2 &point);
			UIAPI void AddLineToPoint(const Vector2 &point);
			UIAPI void ClosePath();

			UIAPI void AddRect(const Rect &rect);
			UIAPI void AddRoundedRect(const Rect &rect, float radius);
			UIAPI void AddRoundedRect(const Rect &rect, float radiusX, float radiusY);

			UIAPI Rect GetBounds() const;

		private:
			PIMPL<PathInternals> _internals;

			RNDeclareMetaAPI(Path, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIPATH_H_ */

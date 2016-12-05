//
//  RNUIPath.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIPath.h"
#include "RNUIInternals.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Path, Object)


		Path::Path()
		{}
		Path::~Path()
		{}


		void Path::MoveToPoint(const Vector2 &point)
		{
			_internals->path.moveTo(point.x, point.y);
		}
		void Path::AddLineToPoint(const Vector2 &point)
		{
			_internals->path.lineTo(point.x, point.y);
		}
		void Path::ClosePath()
		{
			_internals->path.close();
		}


		Path *Path::WithRect(const Rect &rect)
		{
			Path *path = new Path();
			path->AddRect(rect);
			return path->Autorelease();
		}
		Path *Path::WithRoundedRect(const Rect &rect, float radius)
		{
			Path *path = new Path();
			path->AddRoundedRect(rect, radius);
			return path->Autorelease();
		}
		Path *Path::WithRoundedRect(const Rect &rect, float radiusX, float radiusY)
		{
			Path *path = new Path();
			path->AddRoundedRect(rect, radiusX, radiusY);
			return path->Autorelease();
		}

		void Path::AddRect(const Rect &rect)
		{
			_internals->path.addRect(MakeRect(rect));
		}
		void Path::AddRoundedRect(const Rect &rect, float radius)
		{
			_internals->path.addRoundRect(MakeRect(rect), radius, radius);
		}
		void Path::AddRoundedRect(const Rect &rect, float radiusX, float radiusY)
		{
			_internals->path.addRoundRect(MakeRect(rect), radiusX, radiusY);
		}

		Rect Path::GetBounds() const
		{
			const SkRect &rect = _internals->path.getBounds();
			return Rect(rect.fLeft, rect.fTop, rect.fRight - rect.fLeft, rect.fBottom - rect.fTop);
		}
	}
}

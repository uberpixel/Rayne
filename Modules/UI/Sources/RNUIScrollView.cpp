//
//  RNUIScrollView.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIScrollView.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(ScrollView, View)

		ScrollView::ScrollView()
		{

		}

		ScrollView::~ScrollView()
		{
			
		}

		void ScrollView::Draw(Context *context) const
		{
			View::Draw(context);
		}

		void ScrollView::Update(float delta, Vector2 cursorPosition, bool pressed)
		{

		}
	}
}

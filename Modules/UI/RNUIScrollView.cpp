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

		ScrollView::ScrollView(bool vertical, bool horizontal) : _isScrollEnabled(true), _isScrolling(false), _wasTouched(false), _tapTimer(0.0f), _pixelPerInch(200), _scrollsVertical(vertical), _scrollsHorizontal(horizontal)
		{

		}

		ScrollView::~ScrollView()
		{
			
		}
		
		void ScrollView::SetPixelPerInch(float pixelPerInch)
		{
			_pixelPerInch = pixelPerInch;
		}

		void ScrollView::Update(float delta, Vector2 cursorPosition, bool touched, Vector2 alternativeScrollSpeed)
		{
			Vector2 transformedPosition = ConvertPointFromBase(cursorPosition); //Converts to inside bounds, so reverse that effect below
			transformedPosition.x += GetBounds().x;
			transformedPosition.y += GetBounds().y;
			RN::Rect innerFrame = GetFrame(); //would usually be the bounds, but in this case the bounds are getting scaled...
			innerFrame.x = 0.0f;
			innerFrame.y = 0.0f;
			bool isCursorInside = innerFrame.ContainsPoint(transformedPosition);
			
			if(!_scrollsHorizontal) alternativeScrollSpeed.x = 0.0f;
			if(!_scrollsVertical) alternativeScrollSpeed.y = 0.0f;
			
			if(!isCursorInside)
			{
				touched = false;
				alternativeScrollSpeed = 0.0f;
			}
			
			if(!_isScrollEnabled)
			{
				_scrollSpeed = 0.0f;
				return;
			}
			
			if(!_wasTouched && touched)
			{
				_tapTimer = 0.2f;
			}
			_tapTimer -= delta;
			
			if(_wasTouched && touched)
			{
				Vector2 scrollDistance;
				if(_scrollsHorizontal) scrollDistance.x = transformedPosition.x - _previousCursorPosition.x;
				if(_scrollsVertical) scrollDistance.y = transformedPosition.y - _previousCursorPosition.y;
				
				if(scrollDistance.GetLength() > _pixelPerInch/25.4f*5.0f || (_tapTimer <= 0.0f && scrollDistance.GetLength() > _pixelPerInch/25.4f)) _isScrolling = true;
				
				if(_isScrolling)
				{
					RN::Rect scrollBounds = GetBounds();
					RN::Rect scrollFrame = GetFrame();
					
					if(scrollBounds.x + scrollDistance.x > 0.0f)
					{
						float distanceReduction = 0.0f - scrollBounds.x - scrollDistance.x;
						float reductionFactor = 1.0f + distanceReduction / scrollFrame.width;
						scrollDistance.x *= reductionFactor*0.5f;
					}
					
					if(scrollBounds.x + scrollDistance.x < -scrollBounds.width + scrollFrame.width)
					{
						float distanceReduction = (-scrollBounds.width + scrollFrame.width - scrollBounds.x - scrollDistance.x);
						float reductionFactor = 1.0f - distanceReduction / scrollFrame.width;
						scrollDistance.x *= reductionFactor*0.5f;
					}
					
					if(scrollBounds.y + scrollDistance.y > 0.0f)
					{
						float distanceReduction = 0.0f - scrollBounds.y - scrollDistance.y;
						float reductionFactor = 1.0f + distanceReduction / scrollFrame.height;
						scrollDistance.y *= reductionFactor*0.5f;
					}
					
					if(scrollBounds.y + scrollDistance.y < -scrollBounds.height + scrollFrame.height)
					{
						float distanceReduction = (-scrollBounds.height + scrollFrame.height - scrollBounds.y - scrollDistance.y);
						float reductionFactor = 1.0f - distanceReduction / scrollFrame.height;
						scrollDistance.y *= reductionFactor*0.5f;
					}
					
					scrollBounds.x += scrollDistance.x;
					scrollBounds.y += scrollDistance.y;
					SetBounds(scrollBounds);
					
					_scrollSpeed = scrollDistance/delta;
					_scrollSpeed.x = std::min(std::max(_scrollSpeed.x, -20000.0f), 20000.0f);
					_scrollSpeed.y = std::min(std::max(_scrollSpeed.y, -20000.0f), 20000.0f);
				}
			}
			
			if(!touched && alternativeScrollSpeed.GetLength() > k::EpsilonFloat)
			{
				_scrollSpeed = alternativeScrollSpeed;
				_isScrolling = true;
			}
			
			bool isOutOfBounds[2];
			isOutOfBounds[0] = false;
			isOutOfBounds[1] = false;
			if(!touched && _isScrolling)
			{
				RN::Rect scrollBounds = GetBounds();
				RN::Rect scrollFrame = GetFrame();
				scrollBounds.x += _scrollSpeed.x * delta;
				scrollBounds.y += _scrollSpeed.y * delta;
				
				if(scrollBounds.x > RN::k::EpsilonFloat)
				{
					scrollBounds.x += std::max(std::min(-scrollBounds.x, -5.0f) * delta * 10.0f, -scrollBounds.x);
					isOutOfBounds[0] = true;
				}
				
				if(scrollBounds.x < -scrollBounds.width + scrollFrame.width - RN::k::EpsilonFloat)
				{
					float offset = -scrollBounds.width + scrollFrame.width - scrollBounds.x;
					scrollBounds.x += std::min(std::max(offset, 5.0f) * delta * 10.0f, offset);
					isOutOfBounds[0] = true;
				}
				
				if(scrollBounds.y > RN::k::EpsilonFloat)
				{
					scrollBounds.y += std::max(std::min(-scrollBounds.y, -5.0f) * delta * 10.0f, -scrollBounds.y);
					isOutOfBounds[1] = true;
				}
				
				if(scrollBounds.y < -scrollBounds.height + scrollFrame.height - RN::k::EpsilonFloat)
				{
					float offset = -scrollBounds.height + scrollFrame.height - scrollBounds.y;
					scrollBounds.y += std::min(std::max(offset, 5.0f) * delta * 10.0f, offset);
					isOutOfBounds[1] = true;
				}
				
				SetBounds(scrollBounds);
			}
			
			if(_isScrolling)
			{
				Vector2 deceleration = delta*10000.0f;
				if(isOutOfBounds[0]) deceleration.x *= 5.0f;
				if(isOutOfBounds[1]) deceleration.y *= 5.0f;

				deceleration.x = std::min(deceleration.x, std::abs(_scrollSpeed.x));
				deceleration.y = std::min(deceleration.y, std::abs(_scrollSpeed.y));
				_scrollSpeed.x -= (_scrollSpeed.x > 0.0f)? deceleration.x:-deceleration.x;
				_scrollSpeed.y -= (_scrollSpeed.y > 0.0f)? deceleration.y:-deceleration.y;
				
				if(_scrollSpeed.GetLength() < 0.1f)
				{
					if(!touched && !isOutOfBounds[0] && !isOutOfBounds[1])
					{
						RN::Rect scrollBounds = GetBounds();
						RN::Rect scrollFrame = GetFrame();
						
						_isScrolling = false;
						scrollBounds.x = std::min(scrollBounds.x, 0.0f);
						scrollBounds.x = std::max(scrollBounds.x, -scrollBounds.width + scrollFrame.width);
						scrollBounds.y = std::min(scrollBounds.y, 0.0f);
						scrollBounds.y = std::max(scrollBounds.y, -scrollBounds.height + scrollFrame.height);
						SetBounds(scrollBounds);
					}
					
					_scrollSpeed = 0.0f;
				}
			}
			
			_wasTouched = touched;
			_previousCursorPosition = transformedPosition;
		}
	}
}

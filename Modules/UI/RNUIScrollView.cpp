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

		ScrollView::ScrollView() : _isScrollEnabled(true), _isScrolling(false), _wasTouched(false), _tapTimer(0.0f), _pixelPerInch(200), _scrollSpeed(0.0f)
		{

		}

		ScrollView::~ScrollView()
		{
			
		}
		
		void ScrollView::SetPixelPerInch(float pixelPerInch)
		{
			_pixelPerInch = pixelPerInch;
		}

		void ScrollView::Update(float delta, Vector2 cursorPosition, bool touched, float alternativeScrollSpeed)
		{
			Vector2 transformedPosition = ConvertPointFromBase(cursorPosition); //Converts to inside bounds, so reverse that effect below
			transformedPosition.x += GetBounds().x;
			transformedPosition.y += GetBounds().y;
			RN::Rect innerFrame = GetFrame(); //would usually be the bounds, but in this case the bounds are getting scaled...
			innerFrame.x = 0.0f;
			innerFrame.y = 0.0f;
			bool isCursorInside = innerFrame.ContainsPoint(transformedPosition);
			
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
				float scrollDistance = transformedPosition.y - _previousCursorPosition.y;
				if(std::abs(scrollDistance) > _pixelPerInch/25.4f*5.0f || (_tapTimer <= 0.0f && std::abs(scrollDistance) > _pixelPerInch/25.4f)) _isScrolling = true;
				
				if(_isScrolling)
				{
					RN::Rect scrollBounds = GetBounds();
					RN::Rect scrollFrame = GetFrame();
					
					if(scrollBounds.y + scrollDistance > 0.0f)
					{
						float distanceReduction = 0.0f - scrollBounds.y - scrollDistance;
						float reductionFactor = 1.0f + distanceReduction / scrollFrame.height;
						scrollDistance *= reductionFactor*0.5f;
					}
					
					if(scrollBounds.y + scrollDistance < -scrollBounds.height + scrollFrame.height)
					{
						float distanceReduction = (-scrollBounds.height + scrollFrame.height - scrollBounds.y - scrollDistance);
						float reductionFactor = 1.0f - distanceReduction / scrollFrame.height;
						scrollDistance *= reductionFactor*0.5f;
					}
					
					scrollBounds.y += scrollDistance;
					SetBounds(scrollBounds);
					
					_scrollSpeed = scrollDistance/delta;
					_scrollSpeed = std::min(std::max(_scrollSpeed, -20000.0f), 20000.0f);
				}
			}
			
			if(!touched && std::abs(alternativeScrollSpeed) > k::EpsilonFloat)
			{
				_scrollSpeed = alternativeScrollSpeed;
				_isScrolling = true;
			}
			
			bool isOutOfBounds = false;
			if(!touched && _isScrolling)
			{
				RN::Rect scrollBounds = GetBounds();
				RN::Rect scrollFrame = GetFrame();
				scrollBounds.y += _scrollSpeed * delta;
				
				if(scrollBounds.y > RN::k::EpsilonFloat)
				{
					scrollBounds.y += std::max(std::min(-scrollBounds.y, -5.0f) * delta * 10.0f, -scrollBounds.y);
					isOutOfBounds = true;
				}
				
				if(scrollBounds.y < -scrollBounds.height + scrollFrame.height - RN::k::EpsilonFloat)
				{
					float offset = -scrollBounds.height + scrollFrame.height - scrollBounds.y;
					scrollBounds.y += std::min(std::max(offset, 5.0f) * delta * 10.0f, offset);
					isOutOfBounds = true;
				}
				
				SetBounds(scrollBounds);
			}
			
			if(_isScrolling)
			{
				float deceleration = delta*10000.0f;
				if(isOutOfBounds)
				{
					deceleration *= 5.0f;
				}
				deceleration = std::min(deceleration, std::abs(_scrollSpeed));
				_scrollSpeed -= (_scrollSpeed > 0.0f)? deceleration:-deceleration;
				
				if(std::abs(_scrollSpeed) < 0.1f)
				{
					if(!touched && !isOutOfBounds)
					{
						RN::Rect scrollBounds = GetBounds();
						RN::Rect scrollFrame = GetFrame();
						
						_isScrolling = false;
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

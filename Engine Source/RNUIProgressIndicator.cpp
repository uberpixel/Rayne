//
//  RNUIProgressIndicator.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIProgressIndicator.h"
#include "RNUIStyle.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(ProgressIndicator, View)
		
		ProgressIndicator::ProgressIndicator() :
			_min(0.0),
			_max(1.0),
			_current(0.0),
			_progress(nullptr),
			_dirty(true)
		{
			Style *stylesheet = Style::GetSharedInstance();
			Image *background = stylesheet->ParseImage(stylesheet->GetResourceWithKeyPath<Dictionary>(RNCSTR("progress.bar.background")));
			Image *bar        = stylesheet->ParseImage(stylesheet->GetResourceWithKeyPath<Dictionary>(RNCSTR("progress.bar.foreground")));
			
			_background = new ImageView();
			_background->SetImage(background);
			_background->SetAutoresizingMask(View::AutoresizingFlexibleHeight | View::AutoresizingFlexibleWidth);
			
			_bar = new ImageView();
			_bar->SetImage(bar);
			_bar->SetAutoresizingMask(View::AutoresizingFlexibleHeight | View::AutoresizingFlexibleWidth);
			
			AddSubview(_background);
			AddSubview(_bar);
		}
		
		ProgressIndicator::~ProgressIndicator()
		{
			KickProgress();
			
			_background->Release();
			_bar->Release();
		}
		
		
		void ProgressIndicator::SetMin(double minValue)
		{
			_min   = minValue;
			_dirty = true;
		}
		void ProgressIndicator::SetMax(double maxValue)
		{
			_max   = maxValue;
			_dirty = true;
		}
		void ProgressIndicator::SetProgress(double value)
		{
			_current = value;
			_dirty   = true;
		}
		
		
		void ProgressIndicator::SetProgress(Progress *progress)
		{
			KickProgress();
			
			LockGuard<Object *> lock(this);
			_progress = SafeRetain(progress);
			_dirty = true;
			
			if(_progress)
			{
				_min = 0.0;
				_max = 1.0;
				
				_progress->GetProgressSignal().Connect(std::bind(&ProgressIndicator::UpdateToProgress, this, std::placeholders::_1));
				UpdateToProgress(_progress->GetFractionCompleted());
			}
		}
		void ProgressIndicator::KickProgress()
		{
			LockGuard<Object *> lock(this);
			
			if(_progress)
			{
				_progress->GetProgressSignal().Disconnect(this);
				_progress->Release();
				_progress = nullptr;
			}
		}
		
		void ProgressIndicator::UpdateToProgress(double value)
		{
			LockGuard<Object *> lock(this);
			
			if(_progress)
			{
				_current = _progress->GetFractionCompleted();
				_dirty = true;
			}
		}
		
		
		void ProgressIndicator::Update()
		{
			View::Update();
			
			LockGuard<Object *> lock(this);
			
			if(_dirty)
			{
				Rect frame = GetBounds();
				
				if(_progress && _progress->IsComplete())
				{
					KickProgress();
					_current = 1.0;
				}
				
				double value = std::min(_max, std::max(_min, _current));
				frame.width = frame.width * (value / _max);
				
				_bar->SetFrame(frame);
				_dirty = false;
			}
		}
	}
}

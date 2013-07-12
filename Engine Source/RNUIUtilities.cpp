//
//  RNUIUtilities.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIUtilities.h"
#include "RNKernel.h"

namespace RN
{
	namespace UI
	{
		DebugWidget::DebugWidget() :
			Widget(Rect(10.0f, 10.0f, 180.0f, 220.0f)),
			_fps(60)
		{
			MessageCenter::SharedInstance()->AddObserver(kRNKernelDidEndFrameMessage, &DebugWidget::HandleMessage, this, this);
		
			_fpsLabel = new Label();
			_fpsLabel->SetFrame(Rect(0.0f, 5.0f, 180.0f, 28.0f).Inset(5.0f, 0.0f));
			_fpsLabel->SetNumberOfLines(2);
			
			ContentView()->AddSubview(_fpsLabel->Autorelease());
		}
		
		DebugWidget::~DebugWidget()
		{
			MessageCenter::SharedInstance()->RemoveObserver(this);
		}
		
		
		float DebugWidget::AverageFPS()
		{
			float average = 0.0f;
			for(const float& f : _fps)
			{
				average += f;
			}
			
			average /= _fps.size();
			return average;
		}
		
		void DebugWidget::HandleMessage(Message *message)
		{
			Kernel *kernel = Kernel::SharedInstance();
			float delta = kernel->Delta();
			
			if(delta > 0.0f)
				_fps.push(1.0f / delta);
			
			_fpsLabel->SetText(RNSTR("Frame: %3.4fs\nAvg. FPS: %3.4f", kernel->Delta(), AverageFPS()));
		}
	}
}

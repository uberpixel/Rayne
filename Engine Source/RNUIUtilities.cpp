//
//  RNUIUtilities.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIUtilities.h"
#include "RNKernel.h"
#include "RNRenderer.h"

namespace RN
{
	namespace UI
	{
		DebugWidget::DebugWidget() :
			Widget(Widget::StyleTitled | Widget::StyleClosable, Rect(10.0f, 50.0f, 180.0f, 220.0f)),
			_fps(60)
		{
			SetTitle(RNCSTR("Statistics"));
			
			_fpsLabel = new Label();
			_fpsLabel->SetFrame(Rect(0.0f, 5.0f, 180.0f, 128.0f).Inset(5.0f, 0.0f));
			_fpsLabel->SetNumberOfLines(0);
			
			_fpsCheckbox = Button::WithType(Button::Type::CheckBox);
			_fpsCheckbox->SetFrame(Rect(5.0f, 120.0f, 0.0f, 0.0f));
			_fpsCheckbox->SetTitleForState(RNCSTR("Show avg. FPS"), Control::Normal);
			_fpsCheckbox->SizeToFit();
			_fpsCheckbox->SetSelected(true);
			
			GetContentView()->AddSubview(_fpsCheckbox);
			GetContentView()->AddSubview(_fpsLabel->Autorelease());
			
			MessageCenter::GetSharedInstance()->AddObserver(kRNKernelDidEndFrameMessage, &DebugWidget::HandleMessage, this, this);
		}
		
		DebugWidget::~DebugWidget()
		{
			MessageCenter::GetSharedInstance()->RemoveObserver(this);
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
			Kernel *kernel = Kernel::GetSharedInstance();
			Renderer *renderer = Renderer::GetSharedInstance();
			
			float delta = kernel->GetDelta();
			
			if(delta > 0.0f)
				_fps.push(1.0f / delta);
			
			float fps = _fpsCheckbox->IsSelected() ? AverageFPS() : 1.0f / delta;
			const char *fpsText = _fpsCheckbox->IsSelected() ? "Avg. FPS:" : "FPS:";
			
			_fpsLabel->SetText(RNSTR("Frame: %3.4fs\n\%s %3.4f\nLights: %u\nVertices: %uk", kernel->GetDelta(), fpsText, fps, renderer->GetRenderedLights(), renderer->GetRenderedVertices() / 1000));
		}
	}
}

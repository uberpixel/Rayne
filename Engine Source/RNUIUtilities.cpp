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
			
			_label = new Label();
			_label->SetAutoresizingMask(View::AutoresizingFlexibleHeight | View::AutoresizingFlexibleWidth);
			_label->SetNumberOfLines(0);
			
			_fpsCheckbox = Button::WithType(Button::Type::CheckBox);
			_fpsCheckbox->SetAutoresizingMask(View::AutoresizingFlexibleTopMargin);
			_fpsCheckbox->SetTitleForState(RNCSTR("Show avg. FPS"), Control::Normal);
			_fpsCheckbox->SizeToFit();
			_fpsCheckbox->SetSelected(true);
			
			// Layout
			Vector2 checkBoxSize = _fpsCheckbox->GetSizeThatFits();
			Vector2 size = GetFrame().Size();
			
			_fpsCheckbox->SetFrame(Rect(5.0f, size.y - checkBoxSize.y - 5.0f, checkBoxSize.x, checkBoxSize.y));
			_label->SetFrame(Rect(0.0f, 0.0f, size.x, size.y - checkBoxSize.y).Inset(5.0f, 5.0f));
			
			// Misc
			GetContentView()->AddSubview(_fpsCheckbox);
			GetContentView()->AddSubview(_label->Autorelease());
			
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
			//Renderer *renderer = Renderer::GetSharedInstance();
			
			float delta = kernel->GetDelta();
			
			if(delta > 0.0f)
				_fps.push(1.0f / delta);
			
			float fps = _fpsCheckbox->IsSelected() ? AverageFPS() : 1.0f / delta;
			String *string = RNSTR("Frame: %3.4fs\n\%s %3.4f\n\n", kernel->GetDelta(), (_fpsCheckbox->IsSelected() ? "Avg. FPS:" : "FPS:"), fps);
		
			
			auto data = kernel->GetStatisticsData();
			for(Statistics::DataPoint *point : data)
			{
				float milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(point->duration).count() / 1000.0f;
				string->Append(RNSTR("%s: %3.4fs\n", point->name.c_str(), milliseconds));
			}
			
			_label->SetText(string);
		}
	}
}

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
			Widget(Rect(10.0f, 10.0f, 180.0f, 220.0f)),
			_fps(60)
		{
			MessageCenter::SharedInstance()->AddObserver(kRNKernelDidEndFrameMessage, &DebugWidget::HandleMessage, this, this);
		
			_fpsLabel = new Label();
			_fpsLabel->SetFrame(Rect(0.0f, 5.0f, 180.0f, 128.0f).Inset(5.0f, 0.0f));
			_fpsLabel->SetNumberOfLines(0);
			
			ContentView()->AddSubview(_fpsLabel->Autorelease());
			
			_fpsCheckbox = Button::WithType(Button::Type::CheckBox);
			_fpsCheckbox->SetFrame(Rect(5.0f, 120.0f, 0.0f, 0.0f));
			_fpsCheckbox->SetTitleForState(RNCSTR("Show avg. FPS"), Control::Normal);
			_fpsCheckbox->SizeToFit();
			_fpsCheckbox->SetSelected(true);
			
			ContentView()->AddSubview(_fpsCheckbox);
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
			Renderer *renderer = Renderer::SharedInstance();
			
			float delta = kernel->Delta();
			
			if(delta > 0.0f)
				_fps.push(1.0f / delta);
			
			float fps = _fpsCheckbox->IsSelected() ? AverageFPS() : 1.0f / delta;
			const char *fpsText = _fpsCheckbox->IsSelected() ? "Avg. FPS:" : "FPS:";
			
			_fpsLabel->SetText(RNSTR("Frame: %3.4fs\n\%s %3.4f\nLights: %u\nVertices: %uk", kernel->Delta(), fpsText, fps, renderer->RenderedLights(), renderer->RenderedVertices() / 1000));
		}
	}
}

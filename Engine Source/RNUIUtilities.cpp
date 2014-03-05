//
//  RNUIUtilities.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIUtilities.h"
#include "RNKernel.h"
#include "RNRenderer.h"
#include "RNLogging.h"

namespace RN
{
	namespace UI
	{
		DebugWidget::DebugWidget() :
			Widget(Widget::StyleTitled | Widget::StyleClosable, Rect(10.0f, 50.0f, 180.0f, 220.0f)),
			_fps(60)
		{
			SetTitle(RNCSTR("Statistics"));
			SetWidgetLevel(kRNUIWidgetLevelFloating);
			
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
			String *string = RNSTR("Frame: %3.4fs\n%s %3.4f\n\n", kernel->GetDelta(), (_fpsCheckbox->IsSelected() ? "Avg. FPS:" : "FPS:"), fps);
			
			auto data = kernel->GetStatisticsData();
			for(Statistics::DataPoint *point : data)
			{
				float milliseconds = std::chrono::duration_cast<std::chrono::microseconds>(point->duration).count() / 1000000.0f;
				string->Append(RNSTR("%s: %3.4fs\n", point->name.c_str(), milliseconds));
			}
			
			_label->SetText(string);
		}
		
		
		
		ConsoleWidget::ConsoleWidget() :
			Widget(Widget::StyleTitled | Widget::StyleClosable, Rect(180.0f, 50.0f, 480.0f, 320.0f)),
			_messages(512)
		{
			_engine = new Log::CallbackLoggingEngine(this);
			
			SetTitle(RNCSTR("Console"));
			SetWidgetLevel(kRNUIWidgetLevelFloating);
			
			Log::Logger::GetSharedInstance()->AddLoggingEngine(_engine);
			
			Rect frame = GetFrame();
			
			_table = new TableView();
			_table->SetDataSource(this);
			_table->SetFrame(Rect(0.0f, 0.0f, frame.width, frame.height));
			_table->SetAutoresizingMask(View::AutoresizingFlexibleHeight | View::AutoresizingFlexibleWidth);
			
			GetContentView()->AddSubview(_table);
		}
		
		ConsoleWidget::~ConsoleWidget()
		{
			Log::Logger::GetSharedInstance()->RemoveLoggingEngine(_engine);
			_engine->Release();
		}
		
		
		size_t ConsoleWidget::TableViewNumberOfRows(TableView *tableView)
		{
			return _messages.size();
		}
		
		TableViewCell *ConsoleWidget::TableViewCellForRow(TableView *tableView, size_t row)
		{
			TableViewCell *cell = tableView->DequeCellWithIdentifier(RNCSTR("cell"));
			if(!cell)
			{
				cell = new TableViewCell(RNCSTR("cell"));
				cell->Autorelease();
			}
			
			cell->GetTextLabel()->SetText(RNSTR(_messages.at(row).c_str()));
			
			return cell;
		}
		
		void ConsoleWidget::Update()
		{
			CommitQueue();
			Widget::Update();
		}
		
		void ConsoleWidget::CommitQueue()
		{
			Widget::Lock();
			
			if(!_queue.empty())
			{
				_table->BegindEditing();
			
				for(const std::string& message : _queue)
				{
					bool added = (_messages.size() != _messages.capacity());
					_messages.push(std::move(message));
					
					if(!added)
					{
						Range range = _table->GetVisibleRange();
						_table->UpdateRows(range.origin, range.length);
					}
					else
					{
						_table->InsertRows(_messages.size() - 1, 1);
					}
				}
				
				_queue.clear();
				_table->EndEditing();
				
				_table->ScrollToRow(_messages.size(), TableView::ScrollPosition::Bottom);
			}
			
			Widget::Unlock();
		}
		
		
		void ConsoleWidget::SetLogLevel(Log::Level level)
		{
			_engine->SetLevel(level);
		}
		
		void ConsoleWidget::Open()
		{}
		void ConsoleWidget::Close()
		{}
		bool ConsoleWidget::IsOpen() const
		{
			return true;
		}
		
		void ConsoleWidget::CutOff()
		{}
		
		void ConsoleWidget::Write(const Log::Message& message)
		{
			Widget::Lock();
			
			std::stringstream formatted;
			formatted << message.GetFormattedTime() << ": " << message.GetMessage();
			
			_queue.push_back(formatted.str());
			_dirty = true;
			
			Widget::Unlock();
		}
	}
}

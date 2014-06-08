//
//  RNWidget.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIWIDGET_H__
#define __RAYNE_UIWIDGET_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNRect.h"
#include "RNMatrix.h"
#include "RNRenderer.h"
#include "RNVector.h"
#include "RNEnum.h"
#include "RNUIResponder.h"

#define kRNUIWidgetLevelNormal     5
#define kRNUIWidgetLevelBackground 0
#define kRNUIWidgetLevelFloating   15
#define kRNUIWidgetLevelPanel      10

namespace RN
{
	namespace UI
	{
		class Server;
		class View;
		class WidgetBackgroundView;
		
		struct WidgetInternals;
		
		class Widget : public Responder
		{
		public:
			friend class Server;
			friend class View;
			
			struct Style : public Enum<int32>
			{
			public:
				Style()
				{}
				Style(int value) :
					Enum(value)
				{}
				
				enum
				{
					Borderless  = 0,
					Titled      = (1 << 0),
					Closable    = (1 << 1),
					Minimizable = (1 << 2),
					Maximizable = (1 << 3)
				};
			};
			
			enum class TitleControl
			{
				Close,
				Minimize,
				Maximize
			};
			
			struct Delegate
			{
				RNAPI virtual bool WidgetCanClose(Widget *widget) { return true; }
				RNAPI virtual void WidgetWillClose(Widget *widget) {}
				RNAPI virtual void WidgetDidClose(Widget *widget) {}
				
				RNAPI virtual void WidgetDidOpen(Widget *widget) {}
				
				RNAPI virtual bool WidgetCanBecomeKey(Widget *widget) { return true; }
				RNAPI virtual bool WidgetCanResignKey(Widget *widget) { return true; }
				RNAPI virtual void WidgetDidBecomeKey(Widget *widget) {}
				RNAPI virtual void WidgetDidResignKey(Widget *widget) {}
				
				RNAPI virtual void WidgetLayoutContent(Widget *widget) {}
			};
			
			RNAPI Widget(Style style);
			RNAPI Widget(Style style, const Rect &frame);
			RNAPI ~Widget() override;
			
			RNAPI void SetContentView(View *view);
			RNAPI void SetMinimumSize(const Vector2 &size);
			RNAPI void SetMaximumSize(const Vector2 &size);
			RNAPI void SetHasShadow(bool hasShadow);
			RNAPI void SetFrame(const Rect &frame);
			RNAPI void SetContentSize(const Vector2 &size);
			RNAPI void SetTitle(String *title);
			RNAPI void SetTransform(const Matrix &transform);
			RNAPI void SetWidgetLevel(int32 level);
			RNAPI void SetCanBecomeKeyWidget(bool canBecome);
			RNAPI void SetDelegate(Delegate *delegate);
			
			RNAPI const Rect &GetFrame() const { return _frame; }
			RNAPI Vector2 GetContentSize() const;
			
			RNAPI View *GetContentView() const;
			
			RNAPI void SetNeedsLayoutUpdate();
			
			RNAPI bool MakeKeyWidget();
			RNAPI bool MakeFirstResponder(Responder *responder);
			RNAPI Responder *GetFirstResponder() const;
			
			RNAPI void Open();
			RNAPI void Close();
			RNAPI bool IsOpen() const { return _server != nullptr; }
			RNAPI void Center();
			
			RNAPI void OrderFront();
			
			RNAPI Responder *GetNextResponder() const override;
			
		protected:
			RNAPI virtual void Update();
			
		private:
			void ConstraintFrame();
			void ConstraintContentView();
			
			void Render(Renderer *renderer);
			
			bool CanBecomeKeyWidget() const;
			bool CanResignKeyWidget() const;
			void AcceptKey();
			void ResignKey();
			
			void ForceResignFirstResponder();
			View *PerformHitTest(const Vector2 &position, Event *event);
			
			View *CreateContentView();
			WidgetBackgroundView *CreateBackgroundView();
			
			void UpdateLayout();
			
			PIMPL<WidgetInternals> _internals;
			
			WidgetBackgroundView *_backgroundView;
			View *_contentView;
			
			Delegate *_delegate;
			
			int32 _level;
			Server *_server;
			Rect _frame;
			Matrix _transform;
			
			RNDeclareMeta(Widget)
		};
	}
}

#endif /* __RAYNE_UIWIDGET_H__ */

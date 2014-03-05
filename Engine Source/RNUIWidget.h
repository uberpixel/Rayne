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
		
		class Widget : public Responder
		{
		public:
			friend class Server;
			friend class View;
			
			enum
			{
				StyleBorderless  = 0,
				StyleTitled      = (1 << 0),
				StyleClosable    = (1 << 1),
				StyleMinimizable = (1 << 2),
				StyleMaximizable = (1 << 3)
			};
			typedef uint32 Style;
			
			enum class TitleControl
			{
				Close,
				Minimize,
				Maximize
			};
			
			RNAPI Widget(Style style);
			RNAPI Widget(Style style, const Rect& frame);
			RNAPI ~Widget() override;
			
			RNAPI void SetContentView(View *view);
			RNAPI void SetMinimumSize(const Vector2& size);
			RNAPI void SetMaximumSize(const Vector2& size);
			RNAPI void SetHasShadow(bool hasShadow);
			RNAPI void SetFrame(const Rect& frame);
			RNAPI void SetContentSize(const Vector2& size);
			RNAPI void SetTitle(String *title);
			RNAPI void SetTransform(const Matrix& transform);
			RNAPI void SetWidgetLevel(int32 level);
			
			RNAPI const Rect& GetFrame() const { return _frame; }
			RNAPI Vector2 GetContentSize() const;
			
			RNAPI View *GetContentView() const { return _contentView; }
			
			RNAPI void SetNeedsLayoutUpdate();
			
			RNAPI bool MakeFirstResponder(Responder *responder);
			RNAPI Responder *GetFirstResponder() const { return _firstResponder; }
			
			RNAPI void Open();
			RNAPI void Close();
			RNAPI bool IsOpen() const { return _server != nullptr; }
			
			RNAPI void OrderFront();
			
		protected:
			RNAPI virtual void Update();
			
		private:
			void Initialize(Style style);
			void ConstraintFrame();
			void ConstraintContentView();
			
			void Render(Renderer *renderer);
			
			void ForceResignFirstResponder();
			View *PerformHitTest(const Vector2& position, Event *event);
			
			View *CreateContentView();
			WidgetBackgroundView *CreateBackgroundView();
			
			void UpdateLayout();
			
			Style _style;
			bool _hasShadow;
			int32 _level;
			
			Rect _frame;
			
			WidgetBackgroundView *_backgroundView;
			View *_contentView;
			
			Responder *_firstResponder;
			
			Vector2 _minimumSize;
			Vector2 _maximumSize;
			
			Matrix _transform;
			Server *_server;
			
			RNDeclareMeta(Widget, Responder)
		};
	}
}

#endif /* __RAYNE_UIWIDGET_H__ */

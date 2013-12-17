//
//  RNWidget.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
			
			Widget(Style style);
			Widget(Style style, const Rect& frame);
			~Widget() override;
			
			void SetContentView(View *view);
			void SetMinimumSize(const Vector2& size);
			void SetMaximumSize(const Vector2& size);
			void SetHasShadow(bool hasShadow);
			void SetFrame(const Rect& frame);
			void SetContentSize(const Vector2& size);
			void SetTitle(String *title);
			
			const Rect& GetFrame() const { return _frame; }
			Vector2 GetContentSize() const;
			
			View *GetContentView() const { return _contentView; }
			
			void SetNeedsLayoutUpdate();
			
			bool MakeFirstResponder(Responder *responder);
			Responder *GetFirstResponder() const { return _firstResponder; }
			
			void Show();
			void Close();
			
			void OrderFront();
			
		protected:
			Matrix transform;
			
		private:
			void Initialize(Style style);
			void ConstraintFrame();
			void ConstraintContentView();
			
			void Update();
			void Render(Renderer *renderer);
			
			void ForceResignFirstResponder();
			View *PerformHitTest(const Vector2& position, Event *event);
			
			View *GetEmptyContentView();
			WidgetBackgroundView *CreateBackgroundView();
			
			void UpdateLayout();
			
			Style _style;
			bool _hasShadow;
			
			Rect _frame;
			bool _dirtyLayout;
			
			WidgetBackgroundView *_backgroundView;
			View *_contentView;
			
			Responder *_firstResponder;
			
			Vector2 _minimumSize;
			Vector2 _maximumSize;
			
			Matrix _finalTransform;
			Server *_server;
			
			RNDefineMeta(Widget, Responder)
		};
	}
}

#endif /* __RAYNE_UIWIDGET_H__ */

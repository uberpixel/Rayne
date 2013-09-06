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
		
		class Widget : public Responder
		{
		friend class Server;
		friend class View;
		public:
			Widget();
			Widget(const Rect& frame);
			~Widget() override;
			
			virtual void SetContentView(View *view);
			void SetMinimumSize(const Vector2& size);
			void SetMaximumSize(const Vector2& size);
			
			virtual void SetFrame(const Rect& frame);
			virtual void SetContentSize(const Vector2& size);
			
			const Rect& Frame() const { return _frame; }
			Vector2 ContentSize() const;
			
			View *ContentView() const { return _contentView; }
			
			void NeedsLayoutUpdate();
			
			void Show();
			void Close();
			
			void OrderFront();
			
		protected:
			void Render(Renderer *renderer);
			
			Matrix transform;
			
		private:
			void Initialize();
			void ConstraintFrame();
			void ConstraintContentView();
			
			View *EmptyContentView();
			void UpdateLayout();
			
			Rect _frame;
			bool _dirtyLayout;
			
			View *_contentView;
			Vector2 _minimumSize;
			Vector2 _maximumSize;
			
			Matrix _finalTransform;
			Server *_server;
			
			RNDefineMetaWithTraits(Widget, Responder, MetaClassTraitCronstructable)
		};
	}
}

#endif /* __RAYNE_UIWIDGET_H__ */

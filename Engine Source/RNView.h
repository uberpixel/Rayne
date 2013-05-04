//
//  RNView.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VIEW_H__
#define __RAYNE_VIEW_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNResponder.h"
#include "RNMatrix.h"
#include "RNArray.h"
#include "RNRect.h"
#include "RNCamera.h"
#include "RNRenderer.h"

namespace RN
{
	class Widget;
	class View : public Responder
	{
	friend class Widget;
	public:
		View();
		View(const Rect& frame);
		virtual ~View();
		
		const Rect& Frame() const;
		const Rect& Bounds() const;
		
		void SetFrame(const Rect& frame);
		
		void AddSubview(View *subview);
		void RemoveSubview(View *subview);
		void RemoveAllSubviews();
		void RemoveFromSuperview();
		
		void NeedsLayoutUpdate();
		
	protected:
		virtual void Update();
		virtual bool Render(RenderingObject& object);
		
		Material *DrawMaterial() { return _material; }
		
	private:
		void Initialize();
		void ViewHierarchyChanged();
		void Render(Renderer *renderer);
		
		View *_superview;
		Widget *_widget;
		Material *_material;
		
		Array<View *> _subviews;
		
		bool _dirtyLayout;
		
		Rect _frame;
		Matrix _transform;
		Matrix _finalTransform;
		
		RNDefineMeta(View, Responder)
	};
}

#endif /* __RAYNE_VIEW_H__ */

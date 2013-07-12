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
#include "RNMatrix.h"
#include "RNArray.h"
#include "RNRect.h"
#include "RNCamera.h"
#include "RNRenderer.h"
#include "RNInput.h"
#include "RNUIResponder.h"

namespace RN
{
	namespace UI
	{
		class Widget;
		class View : public Responder
		{
		friend class Widget;
		public:
			View();
			View(const Rect& frame);
			~View() override;
			
			Vector2 ConvertPointToView(View *view, const Vector2& point);
			Vector2 ConvertPointFromView(View *view, const Vector2& point);
			
			Rect ConvertRectToView(View *view, const Rect& frame);
			
			const Rect& Frame() const { return _frame; }
			const Rect Bounds() const;
			
			virtual void SetFrame(const Rect& frame);
			void SetBackgroundColor(const Color& color);
			void SetInteractionEnabled(bool enabled);
			
			void AddSubview(View *subview);
			void RemoveSubview(View *subview);
			void RemoveAllSubviews();
			void RemoveFromSuperview();
			
			void NeedsLayoutUpdate();
			
			void SizeToFit();
			virtual Vector2 SizeThatFits();
			
			View *HitTest(const Vector2& point, Event *event);
			virtual bool PointInside(const Vector2& point, Event *event);
			
		protected:
			Mesh *BasicMesh(const Vector2& size);
			Material *DrawMaterial() { return _material; }
			
			virtual void Update();
			virtual bool Render(RenderingObject& object);
			virtual void Render(Renderer *renderer);
			void RenderChilds(Renderer *renderer);
			
			Matrix transform;
			Matrix _finalTransform;
			
		private:
			void Initialize();
			void ViewHierarchyChanged();
			
			void PrepareRendering(RenderingObject& object);
			
			View *_superview;
			Widget *_widget;
			Material *_material;

			Array _subviews;
			
			bool _interactionEnabled;
			bool _dirtyLayout;			
			Rect _frame;
			
			Matrix _intermediateTransform;
			
			RNDefineMetaWithTraits(View, Responder, MetaClassTraitCronstructable)
		};
	}
}

#endif /* __RAYNE_VIEW_H__ */

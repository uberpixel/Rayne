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
#include "RNUIWidget.h"
#include "RNUIResponder.h"

namespace RN
{
	namespace UI
	{
		class View : public Responder
		{
		friend class Widget;
		public:
			View();
			View(const Rect& frame);
			~View() override;
			
			Vector2 ConvertPointToView(const Vector2& point, View *view);
			Vector2 ConvertPointFromView(const Vector2& point, View *view);
			
			Rect ConvertRectToView(const Rect& frame, View *view);
			Rect ConvertRectFromView(const Rect& frame, View *view);
			
			const Rect& Frame() const { return _frame; }
			const Rect& Bounds() const { return _bounds; }
			
			virtual void SetFrame(const Rect& frame);
			virtual void SetBounds(const Rect& bounds);
			
			void SetBackgroundColor(const RN::Color& color);
			void SetInteractionEnabled(bool enabled);
			void SetClipSubviews(bool clipping);
			void SetHidden(bool hidden);
			
			void AddSubview(View *subview);
			void RemoveSubview(View *subview);
			void RemoveAllSubviews();
			void RemoveFromSuperview();
			
			Array *Subivews() { return &_subviews; }
			Widget *Container() { return _widget; }
			
			void SetNeedsLayoutUpdate();
			
			void SizeToFit();
			virtual Vector2 SizeThatFits();
			
			View *HitTest(const Vector2& point, Event *event);
			virtual bool PointInside(const Vector2& point, Event *event);
			
			Responder *NextResponder() const override;
			
		protected:
			Mesh *BasicMesh(const Vector2& size);
			Material *DrawMaterial() { return _material; }
			
			void UpdateBasicMesh(Mesh *mesh, const Vector2& size);
			
			virtual void Update();
			virtual void Draw(Renderer *renderer);
			
			void RenderChilds(Renderer *renderer);
			void PopulateRenderingObject(RenderingObject& object);
			
			virtual void DidAddSubview(View *subview);
			virtual void WillRemoveSubview(View *subview);
			virtual void WillMoveToSuperview(View *superview);
			virtual void DidMoveToSuperview(View *superview);
			
			virtual void LayoutSubviews();
			
			Matrix transform;
			
		private:
			void Initialize();
			void ViewHierarchyChanged();
			void CalculateScissorRect();
			void UpdateChilds();
			void UpdateAndDrawChilds(Renderer *renderer);
			
			void ConvertPointToWidget(Vector2& point);
			void ConvertPointFromWidget(Vector2& point);
			
			View *_superview;
			View *_clippingView;
			
			Widget *_widget;
			Material *_material;
			Material *_viewMaterial;
			Mesh *_mesh;

			Array _subviews;
			
			bool _interactionEnabled;
			bool _dirtyLayout;
			bool _clipSubviews;
			bool _hidden;
			
			Rect _frame;
			Rect _bounds;
			
			Rect _scissorRect;
			
			Matrix _intermediateTransform;
			Matrix _finalTransform;
			
			RNDefineMetaWithTraits(View, Responder, MetaClassTraitCronstructable)
		};
	}
}

#endif /* __RAYNE_VIEW_H__ */

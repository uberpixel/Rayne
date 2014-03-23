//
//  RNView.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
#include "RNEnum.h"
#include "RNUIWidget.h"
#include "RNUIResponder.h"
#include "RNUIGeometry.h"

namespace RN
{
	namespace UI
	{
		class View : public Responder
		{
		public:
			friend class Server;
			friend class Widget;
			
			struct AutoresizingMask : public Enum<int32>
			{
				AutoresizingMask()
				{}
				AutoresizingMask(int value) :
					Enum(value)
				{}
				
				enum
				{
					FlexibleWidth  = (1 << 0),
					FlexibleHeight = (1 << 1),
					
					FlexibleLeftMargin   = (1 << 2),
					FlexibleRightMargin  = (1 << 3),
					FlexibleTopMargin    = (1 << 4),
					FlexibleBottomMargin = (1 << 5)
				};
			};
			
			RNAPI View();
			RNAPI View(const Rect& frame);
			RNAPI ~View() override;
			
			RNAPI Vector2 ConvertPointToView(const Vector2& point, View *view);
			RNAPI Vector2 ConvertPointFromView(const Vector2& point, View *view);
			
			RNAPI Vector2 ConvertPointToBase(const Vector2& point);
			RNAPI Vector2 ConvertPointFromBase(const Vector2& point);
			
			RNAPI Rect ConvertRectToView(const Rect& frame, View *view);
			RNAPI Rect ConvertRectFromView(const Rect& frame, View *view);
			
			RNAPI const Rect& GetFrame() const { return _frame; }
			RNAPI const Rect& GetBounds() const { return _bounds; }
			
			RNAPI virtual void SetFrame(const Rect& frame);
			RNAPI virtual void SetBounds(const Rect& bounds);
			
			RNAPI void SetBackgroundColor(const RN::Color& color);
			RNAPI void SetInteractionEnabled(bool enabled);
			RNAPI void SetClipSubviews(bool clipping);
			RNAPI void SetClipInsets(const EdgeInsets& insets);
			RNAPI void SetHidden(bool hidden);
			RNAPI void SetAutoresizingMask(AutoresizingMask mask);
			RNAPI void SetTransform(const Matrix& transform);
			
			RNAPI void AddSubview(View *subview);
			RNAPI void RemoveSubview(View *subview);
			RNAPI void RemoveAllSubviews();
			RNAPI void RemoveFromSuperview();
			RNAPI void BringSubviewToFront(View *subview);
			RNAPI void SendSubviewToBack(View *subview);
			
			RNAPI View *GetSuperview() const { return _superview; }
			RNAPI const Array *GetSubivews() const { return &_subviews; }
			RNAPI Widget *GetWidget() const { return _widget; }
			RNAPI const Matrix& GetTransform() const { return _transform; }
			RNAPI bool IsHidden() const { return _hidden; }
			
			RNAPI void SetNeedsLayoutUpdate();
			
			RNAPI void SizeToFit();
			RNAPI virtual Vector2 GetSizeThatFits();
			
			RNAPI View *HitTest(const Vector2& point, Event *event);
			RNAPI virtual bool IsPointInside(const Vector2& point, Event *event);
			
			RNAPI Responder *GetNextResponder() const override;
			
		protected:
			RNAPI Mesh *BasicMesh(const Vector2& size);
			RNAPI Material *BasicMaterial(Shader *shader);
			
			RNAPI void UpdateBasicMesh(Mesh *mesh, const Vector2& size);
			
			RNAPI virtual void Update();
			RNAPI virtual void Draw(Renderer *renderer);
			
			RNAPI void RenderChilds(Renderer *renderer);
			RNAPI void PopulateRenderingObject(RenderingObject& object);
			
			RNAPI virtual void DidAddSubview(View *subview);
			RNAPI virtual void WillRemoveSubview(View *subview);
			RNAPI virtual void DidBringSubviewToFront(View *subview);
			RNAPI virtual void DidSendSubviewToBack(View *subview);
			RNAPI virtual void WillMoveToSuperview(View *superview);
			RNAPI virtual void DidMoveToSuperview(View *superview);
			
			RNAPI virtual void LayoutSubviews();
			
		private:
			void Initialize();
			void ViewHierarchyChanged();
			void CalculateScissorRect();
			void ResizeSubviewsFromOldSize(const Vector2& oldSize);
			void UpdateRecursively();
			void DrawRecursively(Renderer *renderer);
			
			void ConvertPointToWidget(Vector2& point);
			void ConvertPointFromWidget(Vector2& point);
			
			View *_superview;
			View *_clippingView;
			
			Widget *_widget;
			Material *_material;
			Mesh *_mesh;
			Matrix _transform;

			AutoresizingMask _autoresizingMask;
			Array _subviews;
			
			bool _interactionEnabled;
			bool _dirtyLayout;
			bool _clipSubviews;
			bool _clipWithWidget;
			bool _hidden;
			
			Rect _frame;
			Rect _bounds;
			EdgeInsets _clipInsets;
			
			Rect _scissorRect;
			
			Matrix _intermediateTransform;
			Matrix _finalTransform;
			
			RNDeclareMeta(View)
		};
	}
}

#endif /* __RAYNE_VIEW_H__ */

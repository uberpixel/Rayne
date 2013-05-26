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
			
			Rect ConvertRectToView(View *view, const Rect& frame);
			
			const Rect& Frame() const { return _frame; }
			const Rect& Bounds() const;
			
			void SetFrame(const Rect& frame);
			void SetBackgroundColor(const Color& color);
			
			void AddSubview(View *subview);
			void RemoveSubview(View *subview);
			void RemoveAllSubviews();
			void RemoveFromSuperview();
			
			void NeedsLayoutUpdate();
			
		protected:
			static Mesh *BasicMesh();
			
			virtual void Update();
			virtual bool Render(RenderingObject& object);
			
			void SetScaleWithFrame(bool scale);
			
			Material *DrawMaterial() { return _material; }
			
			Matrix transform;
			
		private:
			void Initialize();
			void ViewHierarchyChanged();
			void Render(Renderer *renderer);
			
			void PrepareRendering(RenderingObject& object);
			
			View *_superview;
			Widget *_widget;
			Material *_material;
			Material *_viewMaterial;
			
			Mesh *_mesh;
			
			Array _subviews;
			
			bool _dirtyLayout;
			bool _scaleWithFrame;
			
			Rect _frame;
			
			Matrix _finalTransform;
			Matrix _intermediateTransform;
			
			RNDefineMeta(View, Responder)
		};
	}
}

#endif /* __RAYNE_VIEW_H__ */

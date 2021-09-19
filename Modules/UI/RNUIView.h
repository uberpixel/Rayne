//
//  RNUIView.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UILAYER_H_
#define __RAYNE_UILAYER_H_

#include "RNUIConfig.h"
#include "RNUIEdgeInsets.h"

namespace RN
{
	namespace UI
	{
		class Window;
		class View : public Entity
		{
		public:
			friend class Window;

			UIAPI View();
			UIAPI View(const Rect &frame);
			UIAPI ~View();

			UIAPI Vector2 ConvertPointToView(const Vector2 &point, View *view) const;
			UIAPI Vector2 ConvertPointFromView(const Vector2 &point, View *view) const;

			UIAPI Vector2 ConvertPointToBase(const Vector2 &point) const;
			UIAPI Vector2 ConvertPointFromBase(const Vector2 &point) const;

			UIAPI Rect ConvertRectToView(const Rect &frame, View *view) const;
			UIAPI Rect ConvertRectFromView(const Rect &frame, View *view) const;

			const Rect &GetFrame() const { return _frame; }
			const Rect &GetBounds() const { return _bounds; }

			UIAPI void AddSubview(View *subview);
			UIAPI void RemoveSubview(View *subview);
			UIAPI void RemoveAllSubviews();
			UIAPI void RemoveFromSuperview();
			UIAPI void BringSubviewToFront(View *subview);
			UIAPI void SendSubviewToBack(View *subview);
			const Array *GetSubviews() const { return _subviews; }

			UIAPI virtual void SetFrame(const Rect &frame);
			UIAPI virtual void SetBounds(const Rect &bounds);
			
			const Rect &GetScissorRect() const { return _scissorRect; }
			
			UIAPI void SetHidden(bool hidden);
			bool GetIsHidden() const { return _isHidden; }

			UIAPI void SetBackgroundColor(const Color &color);
			UIAPI void SetDepthModeAndWrite(DepthMode depthMode, bool writeDepth, float depthOffset);

			UIAPI virtual void Draw();
			
			UIAPI virtual bool UpdateCursorPosition(const Vector2 &cursorPosition);

		protected:
			UIAPI virtual void DidAddSubview(View *subview);
			UIAPI virtual void WillRemoveSubview(View *subview);
			UIAPI virtual void DidBringSubviewToFront(View *subview);
			UIAPI virtual void DidSendSubviewToBack(View *subview);
			UIAPI virtual void WillMoveToSuperview(View *superview);
			UIAPI virtual void DidMoveToSuperview(View *superview);
			
			UIAPI virtual void UpdateModel();
			
			UIAPI void WillUpdate(ChangeSet changeSet) override;
			
			bool _needsMeshUpdate;

		private:
			void ConvertPointToWindow(Vector2 &point) const;
			void ConvertPointFromWindow(Vector2 &point) const;

			void CalculateScissorRect();

			Rect _bounds;
			Rect _frame;

			bool _clipsToBounds;
			bool _isHidden;
			Rect _scissorRect;

			Color _backgroundColor;
			DepthMode _depthMode;
			bool _isDepthWriteEnabled;
			float _depthOffset;

			View *_superview;
			Array *_subviews;

			RNDeclareMetaAPI(View, UIAPI)
		};
	}
}


#endif /* __RAYNE_UILAYER_H_ */

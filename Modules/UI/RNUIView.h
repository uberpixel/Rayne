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
			
			UIAPI virtual void SetHidden(bool hidden);
			bool GetIsHidden() const { return _isHidden || _isHiddenByParent; }

			UIAPI void SetBackgroundColor(const Color &color);
			UIAPI void SetBackgroundColor(const Color &colorTopLeft, const Color &colorTopRight, const Color &colorBottomLeft, const Color &colorBottomRight);
			UIAPI void SetDepthModeAndWrite(DepthMode depthMode, bool writeDepth, float depthFactor, float depthOffset, bool writeColor = true, bool writeAlpha = false);
			UIAPI void SetBlending(BlendFactor sourceFactorRGB, BlendFactor destinationFactorRGB, BlendOperation operationRGB, BlendFactor sourceFactorA, BlendFactor destinationFactorA, BlendOperation operationA);
			UIAPI void SetCornerRadius(Vector4 radius);
			UIAPI Vector4 GetCornerRadius() const { return _cornerRadius; }
			UIAPI void SetOpacity(float opacity);
			UIAPI void MakeCircle();

			UIAPI virtual void Draw(bool isParentHidden);
			
			UIAPI virtual bool UpdateCursorPosition(const Vector2 &cursorPosition);
			
			UIAPI void SetClipToBounds(bool enabled);
			UIAPI void SetRenderPriorityOverride(int32 renderPriority);
			
			UIAPI void SetRenderGroupForAll(uint8 renderGroup);

		protected:
			UIAPI virtual void DidAddSubview(View *subview);
			UIAPI virtual void WillRemoveSubview(View *subview);
			UIAPI virtual void DidBringSubviewToFront(View *subview);
			UIAPI virtual void DidSendSubviewToBack(View *subview);
			UIAPI virtual void WillMoveToSuperview(View *superview);
			UIAPI virtual void DidMoveToSuperview(View *superview);
			
			UIAPI virtual void SetOpacityFromParent(float parentCombinedOpacity);
			
			UIAPI virtual void UpdateModel();
			
			UIAPI void WillUpdate(ChangeSet changeSet) override;
			
			bool _needsMeshUpdate;
			
			bool _inheritRenderSettings; //If this is set, the values below will be overwritten when adding to the parent
			int32 _renderPriorityOverride;
			DepthMode _depthMode;
			bool _isDepthWriteEnabled;
			bool _isColorWriteEnabled;
			bool _isAlphaWriteEnabled;
			float _depthOffset;
			float _depthFactor;
			
			float _opacityFactor;
			float _combinedOpacityFactor;
			
			BlendFactor _blendSourceFactorRGB;
			BlendFactor _blendDestinationFactorRGB;
			BlendOperation _blendOperationRGB;
			BlendFactor _blendSourceFactorA;
			BlendFactor _blendDestinationFactorA;
			BlendOperation _blendOperationA;

		private:
			void ConvertPointToWindow(Vector2 &point) const;
			void ConvertPointFromWindow(Vector2 &point) const;

			void CalculateScissorRect();

			Rect _bounds;
			Rect _frame;

			bool _clipToBounds;
			bool _isHidden;
			bool _isHiddenByParent;
			Rect _scissorRect;

			RN::Vector4 _cornerRadius;
			bool _isCircle;
			Color _backgroundColor[4];
			bool _hasVertexColors;

			View *_superview;
			Array *_subviews;

			RNDeclareMetaAPI(View, UIAPI)
		};
	}
}


#endif /* __RAYNE_UILAYER_H_ */

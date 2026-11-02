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

			UIAPI static void SetDefaultUIShaders(ShaderLibrary *library, const String *vertexShaderName, const String *fragmentShaderName);
			UIAPI static void ApplyDefaultUIShaders(Material *material, Shader::Options *options);

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
			View *GetSuperview() const { return _superview; }

			UIAPI virtual void SetFrame(const Rect &frame);
			UIAPI virtual void SetBounds(const Rect &bounds);

			const Rect &GetScissorRect() const { return _scissorRect; }

			UIAPI virtual void SetHidden(bool hidden);
			bool GetIsHidden() const { return _isHidden || _isHiddenByParent; }

			UIAPI void SetBackgroundColor(const Color &color);
			UIAPI void SetBackgroundColor(const Color &colorTopLeft, const Color &colorTopRight, const Color &colorBottomLeft, const Color &colorBottomRight);
			Color GetBackgroundColor(int index) const { return _backgroundColor[index]; }
			UIAPI void SetDepthModeAndWrite(DepthMode depthMode, bool writeDepth, float depthFactor, float depthOffset, bool writeColor = true, bool writeAlpha = false);
			UIAPI void SetCullMode(CullMode cullMode);
			UIAPI void SetBlending(BlendFactor sourceFactorRGB, BlendFactor destinationFactorRGB, BlendOperation operationRGB, BlendFactor sourceFactorA, BlendFactor destinationFactorA, BlendOperation operationA);
			UIAPI void SetCornerRadius(Vector4 radius);
			UIAPI Vector4 GetCornerRadius() const { return _cornerRadius; }
			UIAPI void SetOpacity(float opacity);
			UIAPI void MakeCircle();
			UIAPI virtual void SetOutline(Color color, float thickness);
			UIAPI void SetMirrorUV(bool mirrorU, bool mirrorV);
			UIAPI void SetUVOffsetAndScale(RN::Vector2 offset, RN::Vector2 scale);
			UIAPI void SetOutlineCacheEnabled(bool enabled);
			UIAPI bool GetOutlineCacheEnabled() const;

			UIAPI virtual void Draw(bool isParentHidden);

			UIAPI virtual bool UpdateCursorPosition(const Vector2 &cursorPosition);

			UIAPI void SetClipToBounds(bool enabled);
			UIAPI void SetClippingEnabled(bool enabled);
			bool GetClippingEnabled() const { return _isClippingEnabled; }
			UIAPI void SetRenderPriorityOverride(int32 renderPriority);
			UIAPI void SetRenderPriorityOffset(int32 offset);
			int32 GetRenderPriorityOffset() const { return _renderPriorityOffset; }
			UIAPI int32 GetMaxRenderPriorityOffset() const;

			UIAPI void SetRenderGroupForAll(uint16 renderGroup);

			UIAPI virtual void HandleButtonClick();
			UIAPI virtual void HandleButtonClickLate();
			UIAPI virtual void NotifyIntrinsicSizeChanged();

		protected:
			UIAPI virtual void DidAddSubview(View *subview);
			UIAPI virtual void WillRemoveSubview(View *subview);
			UIAPI virtual void DidBringSubviewToFront(View *subview);
			UIAPI virtual void DidSendSubviewToBack(View *subview);
			UIAPI virtual void WillMoveToSuperview(View *superview);
			UIAPI virtual void DidMoveToSuperview(View *superview);

			UIAPI virtual void SetOpacityFromParent(float parentCombinedOpacity);

			UIAPI Vector4 GetClippingRect() const;

			UIAPI virtual void UpdateModel();
			UIAPI void UpdateBackgroundModelIfNeeded();
			UIAPI void PrepareForRender() override;

			UIAPI void WillUpdate(ChangeSet changeSet) override;

			bool _inheritRenderSettings; //If this is set, the values below will be overwritten when adding to the parent
			int32 _renderPriorityOverride;
			int32 _renderPriorityOffset;
			DepthMode _depthMode;
			CullMode _cullMode;
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

			bool _hasOutline;
			Color _outlineColor;

		private:
			void ConvertPointToWindow(Vector2 &point) const;
			void ConvertPointFromWindow(Vector2 &point) const;

			void CalculateScissorRect();

			Rect _bounds;
			Rect _frame;

			bool _backgroundModelDirty;

			Vector2 _oldFrameSize; //Used to check if the frame size changed since the last frame, in which case the mesh needs updating. This is delayed to catch multiple changes to the size that end up the same as before

			bool _clipToBounds;
			bool _isClippingEnabled;
			bool _isHidden;
			bool _isHiddenByParent;
			Rect _scissorRect;
			
			Vector2 _uvOffset;
			Vector2 _uvScale;
			bool _mirrorU;
			bool _mirrorV;

			RN::Vector4 _cornerRadius;
			bool _isCircle;
			Color _backgroundColor[4];
			bool _hasBackgroundGradient;
			float _outlineThickness;
			bool _useOutlineCache;

			View *_superview;
			Array *_subviews;

			static ShaderLibrary *_defaultUIShaderLibrary;
			static String *_defaultUIVertexShaderName;
			static String *_defaultUIFragmentShaderName;

			RNDeclareMetaAPI(View, UIAPI)
		};
	} // namespace UI
} // namespace RN


#endif /* __RAYNE_UILAYER_H_ */

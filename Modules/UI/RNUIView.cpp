//
//  RNUIView.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIView.h"

#include <KGMeshGeneratorLoopBlinn.h> //Used to generate the outline for views with outline and rounded corners, which requires handling of overlaps

namespace RN
{
	namespace UI
	{
		ShaderLibrary *View::_defaultUIShaderLibrary = nullptr;
		String *View::_defaultUIVertexShaderName = nullptr;
		String *View::_defaultUIFragmentShaderName = nullptr;

		void View::SetDefaultUIShaders(ShaderLibrary *library, const String *vertexShaderName, const String *fragmentShaderName)
		{
			if(!library || !vertexShaderName || !fragmentShaderName)
			{
				SafeRelease(_defaultUIShaderLibrary);
				SafeRelease(_defaultUIVertexShaderName);
				SafeRelease(_defaultUIFragmentShaderName);
				return;
			}

			SafeRelease(_defaultUIShaderLibrary);
			_defaultUIShaderLibrary = SafeRetain(library);

			SafeRelease(_defaultUIVertexShaderName);
			_defaultUIVertexShaderName = SafeRetain(const_cast<String *>(vertexShaderName));

			SafeRelease(_defaultUIFragmentShaderName);
			_defaultUIFragmentShaderName = SafeRetain(const_cast<String *>(fragmentShaderName));
		}

		void View::ApplyDefaultUIShaders(Material *material, Shader::Options *options)
		{
			ShaderLibrary *shaderLibrary = _defaultUIShaderLibrary;
			const String *vertexShaderName = _defaultUIVertexShaderName;
			const String *fragmentShaderName = _defaultUIFragmentShaderName;

			if(!shaderLibrary) shaderLibrary = Renderer::GetActiveRenderer()->GetDefaultShaderLibrary();
			if(!vertexShaderName) vertexShaderName = RNCSTR("ui_vertex");
			if(!fragmentShaderName) fragmentShaderName = RNCSTR("ui_fragment");

			auto getUIShader = [&](Shader::Type type, Shader::Options *shaderOptions, bool multiview) {
				Shader::Options *realOptions = shaderOptions;
				if(multiview)
				{
					realOptions = shaderOptions->Copy();
					realOptions->EnableMultiview();
				}

				const String *name = (type == Shader::Type::Vertex) ? vertexShaderName : fragmentShaderName;
				Shader *shader = shaderLibrary->GetShaderWithName(name, realOptions);

				if(realOptions != shaderOptions) realOptions->Release();
				return shader;
			};

			material->SetVertexShader(getUIShader(Shader::Type::Vertex, options, false));
			material->SetFragmentShader(getUIShader(Shader::Type::Fragment, options, false));
			material->SetVertexShader(getUIShader(Shader::Type::Vertex, options, true), RN::Shader::UsageHint::Multiview);
			material->SetFragmentShader(getUIShader(Shader::Type::Fragment, options, true), RN::Shader::UsageHint::Multiview);
		}

		namespace
		{
			enum class CornerType : uint8_t
			{
				TopLeft,
				TopRight,
				BottomRight,
				BottomLeft
			};

			struct CornerCacheKey
			{
				float radius;
				float thickness;

				bool operator<(const CornerCacheKey &other) const
				{
					if(radius != other.radius) return radius < other.radius;
					return thickness < other.thickness;
				}
			};

			static float QuantizeValue(float value)
			{
				return std::round(value * 1000.0f) / 1000.0f;
			}

			static KG::TriangleMesh BuildCornerMesh(float radius, float thickness)
			{
				KG::TriangleMesh mesh;
				if(radius <= RN::k::EpsilonFloat) return mesh;

				float innerRadius = std::max(thickness, radius);

				auto addPoint = [](KG::PathSegment &segment, const Vector2 &point)
				{
					KG::Vector2 value;
					value.x = point.x;
					value.y = point.y;
					segment.controlPoints.push_back(value);
				};

				Vector2 outerStart(0.0f, -radius);
				Vector2 outerControl(0.0f, 0.0f);
				Vector2 outerEnd(radius, 0.0f);

				Vector2 innerStart(thickness, -innerRadius);
				Vector2 innerControl(thickness, -thickness);
				Vector2 innerEnd(innerRadius, -thickness);

				KG::Path outer;
				outer.segments.reserve(4);

				KG::PathSegment segment;
				segment.type = KG::PathSegment::TypeBezierQuadratic;
				addPoint(segment, outerStart);
				addPoint(segment, outerControl);
				addPoint(segment, outerEnd);
				outer.segments.push_back(segment);

				segment = KG::PathSegment();
				segment.type = KG::PathSegment::TypeLine;
				addPoint(segment, outerEnd);
				addPoint(segment, innerEnd);
				outer.segments.push_back(segment);

				segment = KG::PathSegment();
				segment.type = KG::PathSegment::TypeLine;
				addPoint(segment, innerEnd);
				addPoint(segment, innerStart);
				outer.segments.push_back(segment);

				segment = KG::PathSegment();
				segment.type = KG::PathSegment::TypeLine;
				addPoint(segment, innerStart);
				addPoint(segment, outerStart);
				outer.segments.push_back(segment);

				KG::PathCollection silhouette;
				silhouette.paths.push_back(outer);

				KG::PathCollection fill;
				if(radius > thickness)
				{
					KG::Path inner;
					inner.segments.reserve(2);

					segment = KG::PathSegment();
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					addPoint(segment, innerStart);
					addPoint(segment, innerControl);
					addPoint(segment, innerEnd);
					inner.segments.push_back(segment);

					segment = KG::PathSegment();
					segment.type = KG::PathSegment::TypeLine;
					addPoint(segment, innerEnd);
					addPoint(segment, innerStart);
					inner.segments.push_back(segment);

					fill.paths.push_back(inner);
				}

				if(thickness > 0.0f && KG::MeshGeneratorLoopBlinn::GetMeshForContours(silhouette, fill, mesh) == KG::MeshGeneratorLoopBlinn::Status::Success)
				{
					return mesh;
				}

				mesh = KG::MeshGeneratorLoopBlinn::GetMeshForPathCollection(silhouette);
				std::vector<float> vertices;
				vertices.reserve(mesh.vertices.size() / 5 * 6);
				for(size_t i = 0; i < mesh.vertices.size(); i += 5)
				{
					vertices.insert(vertices.end(), mesh.vertices.begin() + i, mesh.vertices.begin() + i + 5);
					vertices.push_back(0.0f);
				}
				mesh.vertices.swap(vertices);
				mesh.features.push_back(KG::TriangleMesh::VertexFeatureOutline);
				return mesh;
			}

			static KG::TriangleMesh GetCornerMesh(float radius, float thickness, bool useCache)
			{
				static std::map<CornerCacheKey, KG::TriangleMesh> cache;

				CornerCacheKey key { QuantizeValue(radius), QuantizeValue(thickness) };
				const auto iterator = cache.find(key);
				if(iterator != cache.end()) return iterator->second;

				if(!useCache)
				{
					return BuildCornerMesh(radius, thickness);
				}

				KG::TriangleMesh mesh = BuildCornerMesh(key.radius, key.thickness);
				const auto result = cache.emplace(key, std::move(mesh));
				return result.first->second;
			}
		}

		RNDefineMeta(View, Entity)

		View::View() :
			_clipToBounds(false),
			_isClippingEnabled(true),
			_isHidden(false),
			_isHiddenByParent(false),
			_subviews(new Array()),
			_superview(nullptr),
			_backgroundColor {Color::ClearColor(), Color::ClearColor(), Color::ClearColor(), Color::ClearColor()},
			_hasBackgroundGradient(false),
			_inheritRenderSettings(true),
			_isDepthWriteEnabled(false),
			_isColorWriteEnabled(true),
			_isAlphaWriteEnabled(true),
			_depthMode(DepthMode::GreaterOrEqual),
			_cullMode(CullMode::None),
			_depthOffset(200.0f),
			_depthFactor(50.0f),
			_opacityFactor(1.0f),
			_combinedOpacityFactor(1.0f),
			_blendSourceFactorRGB(BlendFactor::SourceAlpha),
			_blendDestinationFactorRGB(BlendFactor::OneMinusSourceAlpha),
			_blendOperationRGB(BlendOperation::Add),
			_blendSourceFactorA(BlendFactor::One),
			_blendDestinationFactorA(BlendFactor::One),
			_blendOperationA(BlendOperation::Max),
			_cornerRadius(0.0f, 0.0f, 0.0f, 0.0f),
			_isCircle(false),
			_hasOutline(false),
			_outlineThickness(0.0f),
			_useOutlineCache(false),
			_uvScale(1.0f, 1.0f),
			_mirrorU(false),
			_mirrorV(false),
			_renderPriorityOverride(0),
			_renderPriorityOffset(1)
		{
			SetRenderGroup(1 << 7);
			SetRenderPriority(SceneNode::RenderPriority::RenderUI);
			SetNeedsRenderPreparation();
		}

		View::View(const Rect &frame) :
			View()
		{
			SetFrame(frame);
		}

		View::~View()
		{
			Lock();
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->_superview = nullptr;
			}

			SafeRelease(_subviews);
			Unlock();
		}


		// ---------------------
		// MARK: -
		// MARK: Coordinate systems
		// ---------------------

		void View::ConvertPointToWindow(Vector2 &point) const
		{
			const View *view = this;
			while(view->_superview)
			{
				point.x += view->_frame.x + view->_bounds.x;
				point.y += view->_frame.y + view->_bounds.y;

				point = Vector2(view->GetRotation().GetConjugated().GetRotatedVector(Vector3(point)));

				view = view->_superview;
			}

			point.x += view->_bounds.x;
			point.y += view->_bounds.y;
		}

		void View::ConvertPointFromWindow(Vector2 &point) const
		{
			const View *view = this;
			while(view->_superview)
			{
				point.x -= view->_frame.x + view->_bounds.x;
				point.y -= view->_frame.y + view->_bounds.y;

				point = Vector2(view->GetRotation().GetRotatedVector(Vector3(point)));

				view = view->_superview;
			}

			point.x -= view->_bounds.x;
			point.y -= view->_bounds.y;
		}

		Vector2 View::ConvertPointToView(const Vector2 &point, View *view) const
		{
			Vector2 converted = point;
			ConvertPointToWindow(converted);

			if(!view)
				return converted;

			view->ConvertPointFromWindow(converted);
			return converted;
		}

		Vector2 View::ConvertPointFromView(const Vector2 &point, View *view) const
		{
			Vector2 converted = point;

			if(view)
				view->ConvertPointToWindow(converted);

			ConvertPointFromWindow(converted);
			return converted;
		}

		Vector2 View::ConvertPointToBase(const Vector2 &point) const
		{
			Vector2 converted = point;

			const View *view = this;
			while(view)
			{
				converted.x += view->_frame.x + view->_bounds.x;
				converted.y += view->_frame.y + view->_bounds.y;

				converted = Vector2(view->GetRotation().GetRotatedVector(Vector3(converted)));

				view = view->_superview;
			}

			return converted;
		}

		Vector2 View::ConvertPointFromBase(const Vector2 &point) const
		{
			Vector2 converted = point;

			const View *view = this;
			while(view)
			{
				converted.x -= view->_frame.x + view->_bounds.x;
				converted.y -= view->_frame.y + view->_bounds.y;

				converted = Vector2(view->GetRotation().GetRotatedVector(Vector3(converted)));

				view = view->_superview;
			}

			return converted;
		}

		Rect View::ConvertRectToView(const Rect &frame, View *view) const
		{
			Rect converted = frame;
			Vector2 point = ConvertPointToView(Vector2(frame.x, frame.y), view);

			converted.x = point.x;
			converted.y = point.y;

			return converted;
		}

		Rect View::ConvertRectFromView(const Rect &frame, View *view) const
		{
			Rect converted = frame;
			Vector2 point = ConvertPointFromView(Vector2(frame.x, frame.y), view);

			converted.x = point.x;
			converted.y = point.y;

			return converted;
		}

		// ---------------------
		// MARK: -
		// MARK: Subviews
		// ---------------------

		void View::AddSubview(View *subview)
		{
			Lock();
			subview->Retain();

			if(subview->_superview)
				subview->RemoveFromSuperview();

			subview->WillMoveToSuperview(this);

			_subviews->AddObject(subview);
			subview->_superview = this;
			subview->SetPosition(RN::Vector3(_bounds.x + subview->_frame.x, -_bounds.y - subview->_frame.y, 0.0f)); //Update position to respect the new parents bounds

			AddChild(subview);
			subview->SetRenderGroupForAll(GetRenderGroup());
			subview->SetOpacityFromParent(_combinedOpacityFactor);

			subview->CalculateScissorRect();

			subview->DidMoveToSuperview(this);
			subview->Release();

			DidAddSubview(subview);
			Unlock();
		}

		void View::RemoveSubview(View *subview)
		{
			Lock();
			size_t index = _subviews->GetIndexOfObject(subview);
			if(index != kRNNotFound)
			{
				WillRemoveSubview(subview);

				subview->Retain();
				subview->WillMoveToSuperview(nullptr);

				_subviews->RemoveObjectAtIndex(index);
				subview->RemoveFromParent();

				subview->_superview = nullptr;

				subview->DidMoveToSuperview(nullptr);
				subview->Release();
			}
			Unlock();
		}

		void View::RemoveAllSubviews()
		{
			Lock();
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i++)
			{
				View *subview = _subviews->GetObjectAtIndex<View>(i);

				WillRemoveSubview(subview);
				subview->WillMoveToSuperview(nullptr);
				subview->RemoveFromParent();

				subview->_superview = nullptr;

				subview->DidMoveToSuperview(nullptr);
			}

			_subviews->RemoveAllObjects();
			Unlock();
		}

		void View::RemoveFromSuperview()
		{
			if(!_superview)
				return;

			_superview->RemoveSubview(this);
		}

		void View::BringSubviewToFront(View *subview)
		{
			if(subview->_superview == this)
			{
				subview->Retain();

				Lock();
				_subviews->RemoveObject(subview);
				_subviews->AddObject(subview);
				Unlock();

				subview->Release();
				DidBringSubviewToFront(subview);
			}
		}

		void View::SendSubviewToBack(View *subview)
		{
			if(subview->_superview == this)
			{
				subview->Retain();

				Lock();
				if(_subviews->GetCount() > 1)
				{
					_subviews->RemoveObject(subview);
					_subviews->InsertObjectAtIndex(subview, 0);
				}
				Unlock();

				subview->Release();
				DidSendSubviewToBack(subview);
			}
		}

		void View::DidAddSubview(View *subview)
		{}
		void View::WillRemoveSubview(View *subview)
		{}

		void View::DidBringSubviewToFront(View *subview)
		{}
		void View::DidSendSubviewToBack(View *subview)
		{}

		void View::WillMoveToSuperview(View *superview)
		{}
		void View::DidMoveToSuperview(View *superview)
		{}

		void View::WillUpdate(ChangeSet changeSet)
		{
			if(changeSet == ChangeSet::World)
			{
				if(!GetSceneInfo())
				{
					if(_renderPriorityOverride != 0)
					{
						SetRenderPriority(_renderPriorityOverride);
					}

					if(_superview)
					{
						if(_renderPriorityOverride == 0) SetRenderPriority(_superview->GetRenderPriority() + _renderPriorityOffset);
						if(_inheritRenderSettings) _depthMode = _superview->_depthMode;
					}
				}
			}

			SceneNode::WillUpdate(changeSet);
		}

		void View::HandleButtonClick()
		{
			_subviews->Enumerate<View>([](View *view, size_t index, bool &stop) {
				if(!view->GetIsHidden() && view->_combinedOpacityFactor > RN::k::EpsilonFloat) view->HandleButtonClick();
			});
		}

		void View::HandleButtonClickLate()
		{
			_subviews->Enumerate<View>([](View *view, size_t index, bool &stop) {
				if(!view->GetIsHidden() && view->_combinedOpacityFactor > RN::k::EpsilonFloat) view->HandleButtonClickLate();
			});
		}

		void View::NotifyIntrinsicSizeChanged()
		{
			if(_superview) _superview->NotifyIntrinsicSizeChanged();
		}

		// ---------------------
		// MARK: -
		// MARK: Properties
		// ---------------------

		void View::SetFrame(const Rect &frame)
		{
			if(_frame == frame) return;

			Lock();
			_frame = frame;

			RN::Vector3 newPosition(_frame.x, -_frame.y, 0.0f);
			if(_superview)
			{
				newPosition.x = _superview->_bounds.x + newPosition.x;
				newPosition.y = -_superview->_bounds.y + newPosition.y;
			}
			SetPosition(newPosition);

			_bounds.width = frame.width;
			_bounds.height = frame.height;
			Unlock();

			CalculateScissorRect();
		}

		void View::SetBounds(const Rect &bounds)
		{
			if(_bounds == bounds) return;

			Lock();
			_bounds = bounds;

			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->SetPosition(RN::Vector3(_bounds.x + child->_frame.x, -_bounds.y - child->_frame.y, 0.0f));
			}
			Unlock();

			CalculateScissorRect();
		}

		void View::SetHidden(bool hidden)
		{
			Lock();
			_isHidden = hidden;
			Unlock();
		}

		void View::SetBackgroundColor(const Color &color)
		{
			Lock();
			if(_hasBackgroundGradient)
			{
				Unlock();
				SetBackgroundColor(color, color, color, color);
				return;
			}

			_backgroundColor[0] = color;

			RN::Model *model = GetModel();
			if(model)
			{
				Color finalColor = _backgroundColor[0];
				finalColor.a *= _combinedOpacityFactor;

				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetDiffuseColor(finalColor);
				SetDrawableRenderingEnabled(0, finalColor.a >= k::EpsilonFloat);
			}
			Unlock();
		}

		void View::SetBackgroundColor(const Color &colorTopLeft, const Color &colorTopRight, const Color &colorBottomLeft, const Color &colorBottomRight)
		{
			Lock();
			RN::Model *model = GetModel();
			RN_ASSERT(!model || _hasBackgroundGradient, "Background color with gradient has to be set before the view is rendered the first time.");

			_hasBackgroundGradient = true;
			_backgroundColor[0] = colorTopLeft;
			_backgroundColor[1] = colorTopRight;
			_backgroundColor[2] = colorBottomRight;
			_backgroundColor[3] = colorBottomLeft;

			if(model)
			{
				Color finalColor[4];
				finalColor[0] = _backgroundColor[0];
				finalColor[0].a *= _combinedOpacityFactor;
				finalColor[1] = _backgroundColor[1];
				finalColor[1].a *= _combinedOpacityFactor;
				finalColor[2] = _backgroundColor[2];
				finalColor[2].a *= _combinedOpacityFactor;
				finalColor[3] = _backgroundColor[3];
				finalColor[3].a *= _combinedOpacityFactor;

				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetDiffuseColor(finalColor[0]);
				material->SetSpecularColor(finalColor[1]);
				material->SetEmissiveColor(finalColor[2]);
				material->SetAmbientColor(finalColor[3]);

				SetDrawableRenderingEnabled(0, finalColor[0].a + finalColor[1].a + finalColor[2].a + finalColor[3].a >= k::EpsilonFloat);
			}
			Unlock();
		}

		void View::SetOpacity(float opacity)
		{
			Lock();
			_opacityFactor = opacity;
			float parentOpacity = _superview ? _superview->_combinedOpacityFactor : 1.0f;
			Unlock();

			SetOpacityFromParent(parentOpacity);
		}

		void View::SetDepthModeAndWrite(DepthMode depthMode, bool writeDepth, float depthFactor, float depthOffset, bool colorWrite, bool alphaWrite)
		{
			Lock();
			_inheritRenderSettings = false;
			_depthMode = depthMode;
			_isDepthWriteEnabled = writeDepth;
			_isColorWriteEnabled = colorWrite;
			_isAlphaWriteEnabled = alphaWrite;
			_depthOffset = depthOffset;
			_depthFactor = depthFactor;
			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetDepthWriteEnabled(_isDepthWriteEnabled);
				material->SetColorWriteMask(_isColorWriteEnabled, _isColorWriteEnabled, _isColorWriteEnabled, _isAlphaWriteEnabled);
				material->SetDepthMode(_depthMode);
				material->SetPolygonOffset(_isDepthWriteEnabled, _depthFactor, _depthOffset);
			}
			Unlock();
		}
	
		void View::SetMirrorUV(bool mirrorU, bool mirrorV)
		{
			Lock();
			if(_mirrorU == mirrorU && _mirrorV == mirrorV)
			{
				Unlock();
				return;
			}
			
			_mirrorU = mirrorU;
			_mirrorV = mirrorV;
			SetNeedsRenderPreparation();
			Unlock();
		}
	
		void View::SetUVOffsetAndScale(RN::Vector2 offset, RN::Vector2 scale)
		{
			Lock();
			if(_uvOffset == offset && _uvScale == scale)
			{
				Unlock();
				return;
			}
			
			_uvOffset = offset;
			_uvScale = scale;
			SetNeedsRenderPreparation();
			Unlock();
		}

		void View::SetOutlineCacheEnabled(bool enabled)
		{
			if(_useOutlineCache == enabled) return;
			_useOutlineCache = enabled;
		}

		bool View::GetOutlineCacheEnabled() const
		{
			return _useOutlineCache;
		}

		void View::SetCullMode(CullMode cullMode)
		{
			Lock();
			_cullMode = cullMode;
			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetCullMode(cullMode);
			}
			Unlock();
		}

		void View::SetBlending(BlendFactor sourceFactorRGB, BlendFactor destinationFactorRGB, BlendOperation operationRGB, BlendFactor sourceFactorA, BlendFactor destinationFactorA, BlendOperation operationA)
		{
			Lock();
			//_inheritRenderSettings = false;
			_blendSourceFactorRGB = sourceFactorRGB;
			_blendDestinationFactorRGB = destinationFactorRGB;
			_blendOperationRGB = operationRGB;
			_blendSourceFactorA = sourceFactorA;
			_blendDestinationFactorA = destinationFactorA;
			_blendOperationA = operationA;

			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetBlendFactorSource(_blendSourceFactorRGB, _blendSourceFactorA);
				material->SetBlendFactorDestination(_blendDestinationFactorRGB, _blendDestinationFactorA);
				material->SetBlendOperation(_blendOperationRGB, _blendOperationA);
			}
			Unlock();
		}

		void View::SetCornerRadius(Vector4 radius)
		{
			if(radius == _cornerRadius) return;

			_cornerRadius = radius;
			SetNeedsRenderPreparation();
		}

		void View::SetClipToBounds(bool enabled)
		{
			if(_clipToBounds == enabled) return;

			_clipToBounds = enabled;
			CalculateScissorRect();
		}

		void View::SetClippingEnabled(bool enabled)
		{
			if(_isClippingEnabled == enabled) return;

			_isClippingEnabled = enabled;
			CalculateScissorRect();
		}

		void View::SetRenderPriorityOverride(int32 renderPriority)
		{
			_renderPriorityOverride = renderPriority;
			RN_DEBUG_ASSERT(!_superview, "Needs to be called BEFORE adding to a superview to work");
		}

		void View::SetRenderPriorityOffset(int32 offset)
		{
			_renderPriorityOffset = offset;
			RN_DEBUG_ASSERT(!_superview, "Needs to be called BEFORE adding to a superview to work");
		}

		int32 View::GetMaxRenderPriorityOffset() const
		{
			int32 offset = _renderPriorityOffset;
			int32 maxAdditionalOffset = 0;
			GetSubviews()->Enumerate<View>([&](View *view, size_t index, bool &stop) {
				maxAdditionalOffset = std::max(maxAdditionalOffset, view->GetMaxRenderPriorityOffset());
			});
			return offset + maxAdditionalOffset;
		}

		void View::SetRenderGroupForAll(uint16 renderGroup)
		{
			SetRenderGroup(renderGroup);
			GetSubviews()->Enumerate<View>([renderGroup](View *view, size_t index, bool &stop) {
				view->SetRenderGroupForAll(renderGroup);
			});
		}

		void View::SetOpacityFromParent(float parentCombinedOpacity)
		{
			Lock();

			if(Math::Compare(_combinedOpacityFactor, parentCombinedOpacity * _opacityFactor))
			{
				Unlock();
				return;
			}

			_combinedOpacityFactor = parentCombinedOpacity * _opacityFactor;
			_subviews->Enumerate<View>([&](View *view, size_t index, bool &stop) {
				view->SetOpacityFromParent(_combinedOpacityFactor);
			});

			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				if(!_hasBackgroundGradient)
				{
					Color finalColor = _backgroundColor[0];
					finalColor.a *= _combinedOpacityFactor;

					material->SetDiffuseColor(finalColor);
					SetDrawableRenderingEnabled(0, finalColor.a >= k::EpsilonFloat);
				}
				else
				{
					Color finalColor[4];
					finalColor[0] = _backgroundColor[0];
					finalColor[0].a *= _combinedOpacityFactor;
					finalColor[1] = _backgroundColor[1];
					finalColor[1].a *= _combinedOpacityFactor;
					finalColor[2] = _backgroundColor[2];
					finalColor[2].a *= _combinedOpacityFactor;
					finalColor[3] = _backgroundColor[3];
					finalColor[3].a *= _combinedOpacityFactor;

					material->SetDiffuseColor(finalColor[0]);
					material->SetSpecularColor(finalColor[1]);
					material->SetEmissiveColor(finalColor[2]);
					material->SetAmbientColor(finalColor[3]);

					SetDrawableRenderingEnabled(0, finalColor[0].a + finalColor[1].a + finalColor[2].a + finalColor[3].a >= k::EpsilonFloat);
				}
			}
			Unlock();
		}

		void View::MakeCircle()
		{
			RN_ASSERT(!GetModel(), "MakeCircle can only be called before displaying a view for the first time");
			_isCircle = true;
		}

		void View::SetOutline(Color color, float thickness)
		{
			Lock();
			RN::Model *model = GetModel();
			RN_ASSERT(!model || _hasOutline, "SetOutline has to be called before displaying a view for the first time, but can be used to update the values after");

			_hasOutline = true;
			_outlineColor = color;

			if(thickness != _outlineThickness) SetNeedsRenderPreparation();
			_outlineThickness = thickness;

			if(model)
			{
				Color finalColor;
				finalColor = _outlineColor;
				finalColor.a *= _combinedOpacityFactor;

				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetUIOutlineColor(finalColor);

				//TODO: Skip rendering stuff should also depend on the outline being visible or not...
			}
			Unlock();
		}

		// ---------------------
		// MARK: -
		// MARK: Layout
		// ---------------------

		Vector4 View::GetClippingRect() const
		{
			if(!_isClippingEnabled) return Vector4(0.0f, _frame.width, 0.0f, _frame.height);

			return Vector4(_scissorRect.GetLeft(), _scissorRect.GetRight(), _scissorRect.GetTop(), _scissorRect.GetBottom());
		}

		void View::CalculateScissorRect()
		{
			Lock();
			RN::Rect oldScissorRect = _scissorRect;
			if(_superview)
			{
				if(_superview->_clipToBounds)
				{
					RN::Rect parentScissorRect;
					parentScissorRect.x = -_superview->_bounds.x - _frame.x;
					parentScissorRect.y = -_superview->_bounds.y - _frame.y;
					parentScissorRect.width = _superview->_frame.width;
					parentScissorRect.height = _superview->_frame.height;

					RN::Rect parentParentScissorRect = _superview->_scissorRect;
					parentParentScissorRect.x -= _superview->_bounds.x + _frame.x;
					parentParentScissorRect.y -= _superview->_bounds.y + _frame.y;

					float right = std::min(parentScissorRect.GetRight(), parentParentScissorRect.GetRight());
					float bottom = std::min(parentScissorRect.GetBottom(), parentParentScissorRect.GetBottom());

					_scissorRect.x = std::max(parentScissorRect.x, parentParentScissorRect.x);
					_scissorRect.y = std::max(parentScissorRect.y, parentParentScissorRect.y);

					_scissorRect.width = std::max(right - parentScissorRect.x, 0.0f);
					_scissorRect.height = std::max(bottom - parentScissorRect.y, 0.0f);
				}
				else
				{
					_scissorRect = _superview->_scissorRect;
					_scissorRect.x -= _superview->_bounds.x + _frame.x;
					_scissorRect.y -= _superview->_bounds.y + _frame.y;
				}
			}
			else
			{
				_scissorRect.x = 0.0f;
				_scissorRect.y = 0.0f;
				_scissorRect.width = _frame.width;
				_scissorRect.height = _frame.height;
			}

			//Updating all of this tends to be slow, so only do it if the scissor rect actually changed (that didn't work somehow...)
			//if(oldScissorRect != _scissorRect)
			{
				Model *model = GetModel();
				if(model)
				{
					Model::LODStage *lodStage = model->GetLODStage(0);
					for(int i = 0; i < lodStage->GetCount(); i++)
					{
						lodStage->GetMaterialAtIndex(i)->SetUIClippingRect(GetClippingRect());
					}
				}

				size_t count = _subviews->GetCount();
				for(size_t i = 0; i < count; i++)
				{
					View *child = _subviews->GetObjectAtIndex<View>(i);
					child->CalculateScissorRect();
				}
			}

			Unlock();
		}

		bool View::UpdateCursorPosition(const Vector2 &cursorPosition)
		{
			bool needsRedraw = false;

			Lock();
			/*//Copy to prevent multithreading issues with adding/removing/moving subviews
			//TODO: This is called often, instead _subviews should be updated asynchroneously
			Array *subviewsCopy = _subviews->Copy();
			Unlock();*/

			// Update all children
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				if(child->UpdateCursorPosition(cursorPosition))
				{
					needsRedraw = true;
				}
			}

			Unlock();

			//subviewsCopy->Release();

			return needsRedraw;
		}

		void View::UpdateModel()
		{
			Lock();

			Mesh *mesh = nullptr;

			float maxCornerRadius = std::min(_bounds.width, _bounds.height) * 0.5f;
			Vector4 cornerRadius;
			cornerRadius.x = std::max(std::min(_cornerRadius.x, maxCornerRadius), 0.0f);
			cornerRadius.y = std::max(std::min(_cornerRadius.y, maxCornerRadius), 0.0f);
			cornerRadius.z = std::max(std::min(_cornerRadius.z, maxCornerRadius), 0.0f);
			cornerRadius.w = std::max(std::min(_cornerRadius.w, maxCornerRadius), 0.0f);

			if((cornerRadius.x > 0.0f || cornerRadius.y > 0.0f || cornerRadius.z > 0.0f || cornerRadius.w > 0.0f) && !_isCircle)
			{
				size_t vertexCount = 20;
				size_t indexCount = 30;
				size_t vertexPositionSize = (_hasOutline ? 3 : 2);

				const bool shouldGenerateOutlineMesh = (_hasOutline && _outlineThickness > RN::k::EpsilonFloat);

				std::array<KG::TriangleMesh, 4> cornerMeshData;

				if(shouldGenerateOutlineMesh)
				{
					vertexCount += 4 * 4;
					// Corner meshes also own the four fill wedges.
					indexCount = 18 + 4 * 6;

					auto accumulateCorner = [&](size_t index, float radius)
					{
						if(radius <= RN::k::EpsilonFloat) return;
						cornerMeshData[index] = GetCornerMesh(radius, _outlineThickness, _useOutlineCache);

						vertexCount += cornerMeshData[index].vertices.size() / 6;
						indexCount += cornerMeshData[index].indices.size();
					};

					accumulateCorner(0, cornerRadius.y);
					accumulateCorner(1, cornerRadius.w);
					accumulateCorner(2, cornerRadius.z);
					accumulateCorner(3, cornerRadius.x);
				}

				float *vertexPositionBuffer = new float[vertexCount * vertexPositionSize];
				float *vertexUV0Buffer = new float[vertexCount * 2];
				float *vertexUV1Buffer = new float[vertexCount * 3];
				uint32 *indexBuffer = new uint32[indexCount];

				Vector4 innerCornerRadius(
					std::max(_outlineThickness, cornerRadius.x),
					std::max(_outlineThickness, cornerRadius.y),
					std::max(_outlineThickness, cornerRadius.z),
					std::max(_outlineThickness, cornerRadius.w)
				);

				vertexPositionBuffer[0 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[0 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[0 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[1 * vertexPositionSize + 0] = innerCornerRadius.x;
				vertexPositionBuffer[1 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[1 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[2 * vertexPositionSize + 0] = innerCornerRadius.x;
				vertexPositionBuffer[2 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[2 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[3 * vertexPositionSize + 0] = _frame.width - innerCornerRadius.y;
				vertexPositionBuffer[3 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[3 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[4 * vertexPositionSize + 0] = _frame.width - innerCornerRadius.y;
				vertexPositionBuffer[4 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[4 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[5 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[5 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[5 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[6 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[6 * vertexPositionSize + 1] = -innerCornerRadius.y;
				if(_hasOutline) vertexPositionBuffer[6 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[7 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[7 * vertexPositionSize + 1] = -innerCornerRadius.y;
				if(_hasOutline) vertexPositionBuffer[7 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[8 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[8 * vertexPositionSize + 1] = innerCornerRadius.w - _frame.height;
				if(_hasOutline) vertexPositionBuffer[8 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[9 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[9 * vertexPositionSize + 1] = innerCornerRadius.w - _frame.height;
				if(_hasOutline) vertexPositionBuffer[9 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[10 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[10 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[10 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[11 * vertexPositionSize + 0] = _frame.width - innerCornerRadius.w;
				vertexPositionBuffer[11 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[11 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[12 * vertexPositionSize + 0] = _frame.width - innerCornerRadius.w;
				vertexPositionBuffer[12 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[12 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[13 * vertexPositionSize + 0] = innerCornerRadius.z;
				vertexPositionBuffer[13 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[13 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[14 * vertexPositionSize + 0] = innerCornerRadius.z;
				vertexPositionBuffer[14 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[14 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[15 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[15 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[15 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[16 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[16 * vertexPositionSize + 1] = innerCornerRadius.z - _frame.height;
				if(_hasOutline) vertexPositionBuffer[16 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[17 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[17 * vertexPositionSize + 1] = innerCornerRadius.z - _frame.height;
				if(_hasOutline) vertexPositionBuffer[17 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[18 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[18 * vertexPositionSize + 1] = -innerCornerRadius.x;
				if(_hasOutline) vertexPositionBuffer[18 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[19 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[19 * vertexPositionSize + 1] = -innerCornerRadius.x;
				if(_hasOutline) vertexPositionBuffer[19 * vertexPositionSize + 2] = 0.0f;

				for(int i = 0; i < 20; i++)
				{
					vertexUV0Buffer[i * 2 + 0] = vertexPositionBuffer[i * vertexPositionSize + 0] / _frame.width * _uvScale.x + _uvOffset.x;
					vertexUV0Buffer[i * 2 + 1] = -vertexPositionBuffer[i * vertexPositionSize + 1] / _frame.height * _uvScale.y + _uvOffset.y;
					
					if(_mirrorU)
					{
						vertexUV0Buffer[i * 2 + 0] = 1.0f - vertexUV0Buffer[i * 2 + 0];
					}
					
					if(_mirrorV)
					{
						vertexUV0Buffer[i * 2 + 1] = 1.0f - vertexUV0Buffer[i * 2 + 1];
					}

					vertexUV1Buffer[i * 3 + 0] = 0.0f;
					vertexUV1Buffer[i * 3 + 1] = 1.0f;
					vertexUV1Buffer[i * 3 + 2] = 1.0f;
				}

				vertexUV1Buffer[19 * 3 + 0] = 0.0f;
				vertexUV1Buffer[19 * 3 + 1] = 0.0f;
				vertexUV1Buffer[0 * 3 + 0] = 0.5f;
				vertexUV1Buffer[0 * 3 + 1] = 0.0f;
				vertexUV1Buffer[1 * 3 + 0] = 1.0f;
				vertexUV1Buffer[1 * 3 + 1] = 1.0f;

				vertexUV1Buffer[4 * 3 + 0] = 0.0f;
				vertexUV1Buffer[4 * 3 + 1] = 0.0f;
				vertexUV1Buffer[5 * 3 + 0] = 0.5f;
				vertexUV1Buffer[5 * 3 + 1] = 0.0f;
				vertexUV1Buffer[6 * 3 + 0] = 1.0f;
				vertexUV1Buffer[6 * 3 + 1] = 1.0f;

				vertexUV1Buffer[9 * 3 + 0] = 0.0f;
				vertexUV1Buffer[9 * 3 + 1] = 0.0f;
				vertexUV1Buffer[10 * 3 + 0] = 0.5f;
				vertexUV1Buffer[10 * 3 + 1] = 0.0f;
				vertexUV1Buffer[11 * 3 + 0] = 1.0f;
				vertexUV1Buffer[11 * 3 + 1] = 1.0f;

				vertexUV1Buffer[14 * 3 + 0] = 0.0f;
				vertexUV1Buffer[14 * 3 + 1] = 0.0f;
				vertexUV1Buffer[15 * 3 + 0] = 0.5f;
				vertexUV1Buffer[15 * 3 + 1] = 0.0f;
				vertexUV1Buffer[16 * 3 + 0] = 1.0f;
				vertexUV1Buffer[16 * 3 + 1] = 1.0f;

				indexBuffer[0] = 2;
				indexBuffer[1] = 17;
				indexBuffer[2] = 18;

				indexBuffer[3] = 2;
				indexBuffer[4] = 13;
				indexBuffer[5] = 17;

				indexBuffer[6] = 2;
				indexBuffer[7] = 3;
				indexBuffer[8] = 13;

				indexBuffer[9] = 3;
				indexBuffer[10] = 12;
				indexBuffer[11] = 13;

				indexBuffer[12] = 3;
				indexBuffer[13] = 7;
				indexBuffer[14] = 12;

				indexBuffer[15] = 7;
				indexBuffer[16] = 8;
				indexBuffer[17] = 12;

				if(!shouldGenerateOutlineMesh)
				{
					indexBuffer[18] = 0;
					indexBuffer[19] = 1;
					indexBuffer[20] = 19;

					indexBuffer[21] = 14;
					indexBuffer[22] = 15;
					indexBuffer[23] = 16;

					indexBuffer[24] = 4;
					indexBuffer[25] = 5;
					indexBuffer[26] = 6;

					indexBuffer[27] = 9;
					indexBuffer[28] = 10;
					indexBuffer[29] = 11;
				}

				if(shouldGenerateOutlineMesh)
				{
					size_t outlineVertexCursor = 20;
					size_t outlineIndexCursor = 18;

					auto writeVertex = [&](size_t index, const Vector2 &position, const Vector3 &curveUV, float outlineRole)
					{
						vertexPositionBuffer[index * vertexPositionSize + 0] = position.x;
						vertexPositionBuffer[index * vertexPositionSize + 1] = position.y;
						if(_hasOutline) vertexPositionBuffer[index * vertexPositionSize + 2] = outlineRole;

						float u = position.x / _frame.width * _uvScale.x + _uvOffset.x;
						float v = -position.y / _frame.height * _uvScale.y + _uvOffset.y;
						if(_mirrorU) u = 1.0f - u;
						if(_mirrorV) v = 1.0f - v;

						vertexUV0Buffer[index * 2 + 0] = u;
						vertexUV0Buffer[index * 2 + 1] = v;
						vertexUV1Buffer[index * 3 + 0] = curveUV.x;
						vertexUV1Buffer[index * 3 + 1] = curveUV.y;
						vertexUV1Buffer[index * 3 + 2] = curveUV.z;
					};

					auto addOutlineQuad = [&](const Vector2 &outerStart, const Vector2 &outerEnd, const Vector2 &innerStart, const Vector2 &innerEnd)
					{
						size_t vertexOffset = outlineVertexCursor;
						outlineVertexCursor += 4;

						writeVertex(vertexOffset + 0, outerStart, Vector3(0.0f, 1.0f, 1.0f), 1.0f);
						writeVertex(vertexOffset + 1, outerEnd, Vector3(0.0f, 1.0f, 1.0f), 1.0f);
						writeVertex(vertexOffset + 2, innerStart, Vector3(0.0f, 1.0f, 1.0f), 1.0f);
						writeVertex(vertexOffset + 3, innerEnd, Vector3(0.0f, 1.0f, 1.0f), 1.0f);

						size_t indexOffset = outlineIndexCursor;
						outlineIndexCursor += 6;
						indexBuffer[indexOffset + 0] = static_cast<uint32>(vertexOffset + 0);
						indexBuffer[indexOffset + 1] = static_cast<uint32>(vertexOffset + 1);
						indexBuffer[indexOffset + 2] = static_cast<uint32>(vertexOffset + 2);
						indexBuffer[indexOffset + 3] = static_cast<uint32>(vertexOffset + 1);
						indexBuffer[indexOffset + 4] = static_cast<uint32>(vertexOffset + 2);
						indexBuffer[indexOffset + 5] = static_cast<uint32>(vertexOffset + 3);
					};

					auto addCornerMesh = [&](CornerType type, const Vector2 &translation, const KG::TriangleMesh &cornerMesh)
					{
						if(cornerMesh.vertices.empty()) return;

						auto rotatePosition = [&](const Vector2 &position) -> Vector2
						{
							switch(type)
							{
								case CornerType::TopLeft:
									return position;
								case CornerType::TopRight:
									return Vector2(position.y, -position.x);
								case CornerType::BottomRight:
									return Vector2(-position.x, -position.y);
								case CornerType::BottomLeft:
									return Vector2(-position.y, position.x);
							}
							return position;
						};

						const size_t vertexCount = cornerMesh.vertices.size() / 6;
						size_t vertexOffset = outlineVertexCursor;
						for(size_t i = 0; i < vertexCount; i++)
						{
							const float x = cornerMesh.vertices[i * 6 + 0];
							const float y = cornerMesh.vertices[i * 6 + 1];
							const float curveU = cornerMesh.vertices[i * 6 + 2];
							const float curveV = cornerMesh.vertices[i * 6 + 3];
							const float curveDirection = cornerMesh.vertices[i * 6 + 4];
							const float outlineRole = cornerMesh.vertices[i * 6 + 5];

							Vector2 position = rotatePosition(Vector2(x, y)) + translation;
							writeVertex(vertexOffset + i, position, Vector3(curveU, curveV, curveDirection), outlineRole);
						}

						const uint32 baseIndex = static_cast<uint32>(vertexOffset);
						for(uint32_t index : cornerMesh.indices)
						{
							indexBuffer[outlineIndexCursor++] = baseIndex + static_cast<uint32>(index);
						}

						outlineVertexCursor += vertexCount;
					};

					// Top edge
					addOutlineQuad(
						Vector2(cornerRadius.x, 0.0f),
						Vector2(_frame.width - cornerRadius.y, 0.0f),
						Vector2(innerCornerRadius.x, -_outlineThickness),
						Vector2(_frame.width - innerCornerRadius.y, -_outlineThickness));

					// Right edge
					addOutlineQuad(
						Vector2(_frame.width, -cornerRadius.y),
						Vector2(_frame.width, -_frame.height + cornerRadius.w),
						Vector2(_frame.width - _outlineThickness, -innerCornerRadius.y),
						Vector2(_frame.width - _outlineThickness, -_frame.height + innerCornerRadius.w));

					// Bottom edge
					addOutlineQuad(
						Vector2(_frame.width - cornerRadius.w, -_frame.height),
						Vector2(cornerRadius.z, -_frame.height),
						Vector2(_frame.width - innerCornerRadius.w, -_frame.height + _outlineThickness),
						Vector2(innerCornerRadius.z, -_frame.height + _outlineThickness));

					// Left edge
					addOutlineQuad(
						Vector2(0.0f, -_frame.height + cornerRadius.z),
						Vector2(0.0f, -cornerRadius.x),
						Vector2(_outlineThickness, -_frame.height + innerCornerRadius.z),
						Vector2(_outlineThickness, -innerCornerRadius.x));

					addCornerMesh(CornerType::TopRight, Vector2(_frame.width, 0.0f), cornerMeshData[0]);
					addCornerMesh(CornerType::BottomRight, Vector2(_frame.width, -_frame.height), cornerMeshData[1]);
					addCornerMesh(CornerType::BottomLeft, Vector2(0.0f, -_frame.height), cornerMeshData[2]);
					addCornerMesh(CornerType::TopLeft, Vector2(0.0f, 0.0f), cornerMeshData[3]);
				}

				std::vector<Mesh::VertexAttribute> meshVertexAttributes;
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, _hasOutline ? PrimitiveType::Vector3 : PrimitiveType::Vector2);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords1, PrimitiveType::Vector3);

				mesh = new Mesh(meshVertexAttributes, vertexCount, indexCount);
				mesh->BeginChanges();

				mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, vertexUV0Buffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords1, vertexUV1Buffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indexBuffer);

				mesh->EndChanges();

				delete[] vertexPositionBuffer;
				delete[] vertexUV0Buffer;
				delete[] vertexUV1Buffer;
				delete[] indexBuffer;
			}
			else
			{
				size_t vertexCount = _hasOutline ? 12 : 4;
				size_t indexCount = _hasOutline ? 30 : 6;
				size_t vertexPositionSize = (_hasOutline ? 3 : 2);
				float *vertexPositionBuffer = new float[vertexCount * vertexPositionSize];
				float *vertexUVBuffer = new float[vertexCount * 2];
				uint32 *indexBuffer = new uint32[indexCount];

				vertexPositionBuffer[0 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[0 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[0 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[1 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[1 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[1 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[2 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[2 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[2 * vertexPositionSize + 2] = 0.0f;

				vertexPositionBuffer[3 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[3 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[3 * vertexPositionSize + 2] = 0.0f;

				vertexUVBuffer[0 * 2 + 0] = _uvOffset.x;
				vertexUVBuffer[0 * 2 + 1] = _uvOffset.y;
				if(_mirrorU) vertexUVBuffer[0 * 2 + 0] = 1.0f - vertexUVBuffer[0 * 2 + 0];
				if(_mirrorV) vertexUVBuffer[0 * 2 + 1] = 1.0f - vertexUVBuffer[0 * 2 + 1];

				vertexUVBuffer[1 * 2 + 0] = _uvScale.x + _uvOffset.x;
				vertexUVBuffer[1 * 2 + 1] = _uvOffset.y;
				if(_mirrorU) vertexUVBuffer[1 * 2 + 0] = 1.0f - vertexUVBuffer[1 * 2 + 0];
				if(_mirrorV) vertexUVBuffer[1 * 2 + 1] = 1.0f - vertexUVBuffer[1 * 2 + 1];

				vertexUVBuffer[2 * 2 + 0] = _uvScale.x + _uvOffset.x;
				vertexUVBuffer[2 * 2 + 1] = _uvScale.y + _uvOffset.y;
				if(_mirrorU) vertexUVBuffer[2 * 2 + 0] = 1.0f - vertexUVBuffer[2 * 2 + 0];
				if(_mirrorV) vertexUVBuffer[2 * 2 + 1] = 1.0f - vertexUVBuffer[2 * 2 + 1];

				vertexUVBuffer[3 * 2 + 0] = _uvOffset.x;
				vertexUVBuffer[3 * 2 + 1] = _uvScale.y + _uvOffset.y;
				if(_mirrorU) vertexUVBuffer[3 * 2 + 0] = 1.0f - vertexUVBuffer[3 * 2 + 0];
				if(_mirrorV) vertexUVBuffer[3 * 2 + 1] = 1.0f - vertexUVBuffer[3 * 2 + 1];

				indexBuffer[0] = 0;
				indexBuffer[1] = 3;
				indexBuffer[2] = 1;

				indexBuffer[3] = 3;
				indexBuffer[4] = 2;
				indexBuffer[5] = 1;

				if(_hasOutline)
				{
					//Inner vertices of outline
					vertexPositionBuffer[4 * vertexPositionSize + 0] = _outlineThickness;
					vertexPositionBuffer[4 * vertexPositionSize + 1] = -_outlineThickness;
					vertexPositionBuffer[4 * vertexPositionSize + 2] = 1.0f;

					vertexPositionBuffer[5 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
					vertexPositionBuffer[5 * vertexPositionSize + 1] = -_outlineThickness;
					vertexPositionBuffer[5 * vertexPositionSize + 2] = 1.0f;

					vertexPositionBuffer[6 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
					vertexPositionBuffer[6 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
					vertexPositionBuffer[6 * vertexPositionSize + 2] = 1.0f;

					vertexPositionBuffer[7 * vertexPositionSize + 0] = _outlineThickness;
					vertexPositionBuffer[7 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
					vertexPositionBuffer[7 * vertexPositionSize + 2] = 1.0f;

					vertexUVBuffer[4 * 2 + 0] = _uvOffset.x;
					vertexUVBuffer[4 * 2 + 1] = _uvOffset.y;
					if(_mirrorU) vertexUVBuffer[4 * 2 + 0] = 1.0f - vertexUVBuffer[4 * 2 + 0];
					if(_mirrorV) vertexUVBuffer[4 * 2 + 1] = 1.0f - vertexUVBuffer[4 * 2 + 1];

					vertexUVBuffer[5 * 2 + 0] = _uvScale.x + _uvOffset.x;
					vertexUVBuffer[5 * 2 + 1] = _uvOffset.y;
					if(_mirrorU) vertexUVBuffer[5 * 2 + 0] = 1.0f - vertexUVBuffer[5 * 2 + 0];
					if(_mirrorV) vertexUVBuffer[5 * 2 + 1] = 1.0f - vertexUVBuffer[5 * 2 + 1];

					vertexUVBuffer[6 * 2 + 0] = _uvScale.x + _uvOffset.x;
					vertexUVBuffer[6 * 2 + 1] = _uvScale.y + _uvOffset.y;
					if(_mirrorU) vertexUVBuffer[6 * 2 + 0] = 1.0f - vertexUVBuffer[6 * 2 + 0];
					if(_mirrorV) vertexUVBuffer[6 * 2 + 1] = 1.0f - vertexUVBuffer[6 * 2 + 1];

					vertexUVBuffer[7 * 2 + 0] = _uvOffset.x;
					vertexUVBuffer[7 * 2 + 1] = _uvScale.y + _uvOffset.y;
					if(_mirrorU) vertexUVBuffer[7 * 2 + 0] = 1.0f - vertexUVBuffer[7 * 2 + 0];
					if(_mirrorV) vertexUVBuffer[7 * 2 + 1] = 1.0f - vertexUVBuffer[7 * 2 + 1];

					//Outter vertices of outline
					vertexPositionBuffer[8 * vertexPositionSize + 0] = 0.0f;
					vertexPositionBuffer[8 * vertexPositionSize + 1] = 0.0f;
					vertexPositionBuffer[8 * vertexPositionSize + 2] = 1.0f;

					vertexPositionBuffer[9 * vertexPositionSize + 0] = _frame.width;
					vertexPositionBuffer[9 * vertexPositionSize + 1] = 0.0f;
					vertexPositionBuffer[9 * vertexPositionSize + 2] = 1.0f;

					vertexPositionBuffer[10 * vertexPositionSize + 0] = _frame.width;
					vertexPositionBuffer[10 * vertexPositionSize + 1] = -_frame.height;
					vertexPositionBuffer[10 * vertexPositionSize + 2] = 1.0f;

					vertexPositionBuffer[11 * vertexPositionSize + 0] = 0.0f;
					vertexPositionBuffer[11 * vertexPositionSize + 1] = -_frame.height;
					vertexPositionBuffer[11 * vertexPositionSize + 2] = 1.0f;
					
					vertexUVBuffer[8 * 2 + 0] = _uvOffset.x;
					vertexUVBuffer[8 * 2 + 1] = _uvOffset.y;
					if(_mirrorU) vertexUVBuffer[8 * 2 + 0] = 1.0f - vertexUVBuffer[8 * 2 + 0];
					if(_mirrorV) vertexUVBuffer[8 * 2 + 1] = 1.0f - vertexUVBuffer[8 * 2 + 1];

					vertexUVBuffer[9 * 2 + 0] = _uvScale.x + _uvOffset.x;
					vertexUVBuffer[9 * 2 + 1] = _uvOffset.y;
					if(_mirrorU) vertexUVBuffer[9 * 2 + 0] = 1.0f - vertexUVBuffer[9 * 2 + 0];
					if(_mirrorV) vertexUVBuffer[9 * 2 + 1] = 1.0f - vertexUVBuffer[9 * 2 + 1];

					vertexUVBuffer[10 * 2 + 0] = _uvScale.x + _uvOffset.x;
					vertexUVBuffer[10 * 2 + 1] = _uvScale.y + _uvOffset.y;
					if(_mirrorU) vertexUVBuffer[10 * 2 + 0] = 1.0f - vertexUVBuffer[10 * 2 + 0];
					if(_mirrorV) vertexUVBuffer[10 * 2 + 1] = 1.0f - vertexUVBuffer[10 * 2 + 1];

					vertexUVBuffer[11 * 2 + 0] = _uvOffset.x;
					vertexUVBuffer[11 * 2 + 1] = _uvScale.y + _uvOffset.y;
					if(_mirrorU) vertexUVBuffer[11 * 2 + 0] = 1.0f - vertexUVBuffer[11 * 2 + 0];
					if(_mirrorV) vertexUVBuffer[11 * 2 + 1] = 1.0f - vertexUVBuffer[11 * 2 + 1];

					//Top part of the outline
					indexBuffer[6] = 8; //top left
					indexBuffer[7] = 4; //bottom left
					indexBuffer[8] = 9; //top right

					indexBuffer[9] = 4; //bottom left
					indexBuffer[10] = 5; //bottom right
					indexBuffer[11] = 9; //top right

					//Bottom part of the outline
					indexBuffer[12] = 7; //top left
					indexBuffer[13] = 11; //bottom left
					indexBuffer[14] = 6; //top right

					indexBuffer[15] = 11; //bottom left
					indexBuffer[16] = 10; //bottom right
					indexBuffer[17] = 6; //top right

					//Left part of the outline
					indexBuffer[18] = 8; //top left
					indexBuffer[19] = 11; //bottom left
					indexBuffer[20] = 4; //top right

					indexBuffer[21] = 11; //bottom left
					indexBuffer[22] = 7; //bottom right
					indexBuffer[23] = 4; //top right

					//Right part of the outline
					indexBuffer[24] = 5; //top left
					indexBuffer[25] = 6; //bottom left
					indexBuffer[26] = 9; //top right

					indexBuffer[27] = 6; //bottom left
					indexBuffer[28] = 10; //bottom right
					indexBuffer[29] = 9; //top right
				}

				std::vector<Mesh::VertexAttribute> meshVertexAttributes;
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, _hasOutline ? PrimitiveType::Vector3 : PrimitiveType::Vector2);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);

				mesh = new Mesh(meshVertexAttributes, vertexCount, indexCount);
				mesh->BeginChanges();

				mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, vertexUVBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indexBuffer);

				mesh->EndChanges();

				delete[] vertexPositionBuffer;
				delete[] vertexUVBuffer;
				delete[] indexBuffer;
			}

			Model *model = GetModel();
			if(!model)
			{
				Material *material = Material::WithShaders(nullptr, nullptr);
				Shader::Options *shaderOptions = Shader::Options::WithNone();
				shaderOptions->EnableAlpha();
				shaderOptions->AddDefine("RN_UI", "1");
				if((cornerRadius.x > 0.0f || cornerRadius.y > 0.0f || cornerRadius.z > 0.0f || cornerRadius.w > 0.0f) && !_isCircle) shaderOptions->AddDefine("RN_UV1", "1");
				if(_hasBackgroundGradient) shaderOptions->AddDefine("RN_UI_GRADIENT", "1");
				if(_isCircle) shaderOptions->AddDefine("RN_UI_CIRCLE", "1");
				if(_hasOutline) shaderOptions->AddDefine("RN_UI_OUTLINE", "1");
				material->SetAlphaToCoverage(false);
				material->SetCullMode(_cullMode);
				material->SetDepthMode(_depthMode);
				material->SetDepthWriteEnabled(_isDepthWriteEnabled);
				material->SetColorWriteMask(_isColorWriteEnabled, _isColorWriteEnabled, _isColorWriteEnabled, _isAlphaWriteEnabled);
				material->SetPolygonOffset(_isDepthWriteEnabled, _depthFactor, _depthOffset);
				material->SetBlendFactorSource(_blendSourceFactorRGB, _blendSourceFactorA);
				material->SetBlendFactorDestination(_blendDestinationFactorRGB, _blendDestinationFactorA);
				material->SetBlendOperation(_blendOperationRGB, _blendOperationA);
				if(!_hasBackgroundGradient)
				{
					Color finalColor = _backgroundColor[0];
					finalColor.a *= _combinedOpacityFactor;
					SetDrawableRenderingEnabled(0, finalColor.a >= k::EpsilonFloat);
					material->SetDiffuseColor(finalColor);
				}
				else
				{
					Color finalColor[4];
					finalColor[0] = _backgroundColor[0];
					finalColor[0].a *= _combinedOpacityFactor;
					finalColor[1] = _backgroundColor[1];
					finalColor[1].a *= _combinedOpacityFactor;
					finalColor[2] = _backgroundColor[2];
					finalColor[2].a *= _combinedOpacityFactor;
					finalColor[3] = _backgroundColor[3];
					finalColor[3].a *= _combinedOpacityFactor;

					material->SetDiffuseColor(finalColor[0]);
					material->SetSpecularColor(finalColor[1]);
					material->SetEmissiveColor(finalColor[2]);
					material->SetAmbientColor(finalColor[3]);

					SetDrawableRenderingEnabled(0, finalColor[0].a + finalColor[1].a + finalColor[2].a + finalColor[3].a >= k::EpsilonFloat);
				}

				if(_hasOutline)
				{
					Color finalColor;
					finalColor = _outlineColor;
					finalColor.a *= _combinedOpacityFactor;

					material->SetUIOutlineColor(finalColor);

					//TODO: Skip rendering stuff should also depend on the outline being visible or not... see background gradient...
				}

				ApplyDefaultUIShaders(material, shaderOptions);

				material->SetUIClippingRect(GetClippingRect());
				material->SetUIOffset(Vector2(0.0f, 0.0f));

				model = new Model();
				model->AddLODStage(0.05f)->AddMesh(mesh->Autorelease(), material);

				model->Retain();
				SetModel(model->Autorelease());
				model->Release();
			}
			else
			{
				model->GetLODStage(0)->ReplaceMesh(mesh->Autorelease(), 0);
				RefreshDrawableSources();
			}

			model->CalculateBoundingVolumes();
			SetBoundingBox(model->GetBoundingBox());
			Unlock();
		}

		void View::PrepareForRender()
		{
			UpdateModel();
		}

		// ---------------------
		// MARK: -
		// MARK: Drawing
		// ---------------------

		void View::Draw(bool isParentHidden)
		{
			_isHiddenByParent = isParentHidden;
			bool isHidden = _isHidden || isParentHidden || (_isClippingEnabled && !_bounds.IntersectsRect(_scissorRect)) || _combinedOpacityFactor <= k::EpsilonFloat;
			if(isHidden)
			{
				AddFlags(SceneNode::Flags::Hidden);
			}
			else
			{
				RemoveFlags(SceneNode::Flags::Hidden);

				//Update mesh if the frame size changed
				if(_oldFrameSize.GetSquaredDistance(_frame.GetSize()) > k::EpsilonFloat)
				{
					SetNeedsRenderPreparation();
					_oldFrameSize = _frame.GetSize();
				}

				if(!GetModel()) PrepareForRenderIfNeeded();
			}

			// Draw all children
			Lock();
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->Draw(_isHidden || isParentHidden);
			}
			Unlock();
		}
	} // namespace UI
} // namespace RN

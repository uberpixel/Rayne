//
//  RNRenderFrame.h
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERFRAME_H__
#define __RAYNE_RENDERFRAME_H__

#include "RNDrawable.h"
#include "RNGPUBuffer.h"
#include "RNRenderingConfig.h"
#include "RNRenderPass.h"
#include "RNTexture.h"
#include "../Objects/RNObject.h"
#include "../Scene/RNCamera.h"
#include "../Scene/RNLight.h"

namespace RN
{
	class RenderFramePresentationState : public Object
	{
	public:
		RNAPI virtual bool BeginFrameOnRenderThread();
		RNAPI virtual void EndFrameOnRenderThread();
		RNAPI virtual void CancelFrameOnRenderThread();

	protected:
		RNAPI RenderFramePresentationState();
		RNAPI ~RenderFramePresentationState();

		__RNDeclareMetaInternal(RenderFramePresentationState)
	};

	class RenderFrame
	{
	public:
		struct CameraStatistics
		{
			uint64 numberOfDrawables = 0;
			uint64 numberOfDrawCalls = 0;
			uint64 numberOfVertices = 0;
			uint64 numberOfIndices = 0;
		};

		struct InstancingSortKey
		{
			bool CanInstanceWith(const InstancingSortKey &other) const { return isSupported && other.isSupported && hash == other.hash; }
			bool operator<(const InstancingSortKey &other) const
			{
				if(renderPriority != other.renderPriority) return renderPriority < other.renderPriority;
				if(renderPriority >= SceneNode::RenderSky) return submissionIndex < other.submissionIndex;
				if(isSupported != other.isSupported) return isSupported;
				if(!isSupported || hash == other.hash) return submissionIndex < other.submissionIndex;
				return hash < other.hash;
			}

			size_t hash = 0;
			size_t submissionIndex = 0;
			int32 renderPriority = SceneNode::RenderNormal;
			bool isSupported = false;
		};

		class CameraSnapshot
		{
		public:
			static constexpr uint64 InvalidSourceCameraUID = static_cast<uint64>(-1);
			static constexpr size_t FrustumPlaneCount = 6;

			CameraSnapshot() :
				_renderOrigin(),
				_sourceCameraUID(InvalidSourceCameraUID),
				_sortInstancable(false),
				_tag(0)
			{}

		private:
			CameraSnapshot(const PositionType &renderOrigin, uint64 sourceCameraUID, bool sortInstancable, const Matrix &viewMatrix, const Matrix &inverseViewMatrix, const Matrix &projectionMatrix, const Matrix &inverseProjectionMatrix, const Matrix &projectionViewMatrix, const Matrix &inverseProjectionViewMatrix, const Vector4 *frustumPlanes, const Color &ambientColor, const Vector4 &customData, const Color &fogColor0, const Color &fogColor1, const Vector2 &clipDistance, const Vector2 &fogDistance, int32 tag, const Rect &frame) :
				_renderOrigin(renderOrigin),
				_sourceCameraUID(sourceCameraUID),
				_sortInstancable(sortInstancable),
				_viewMatrix(viewMatrix),
				_inverseViewMatrix(inverseViewMatrix),
				_projectionMatrix(projectionMatrix),
				_inverseProjectionMatrix(inverseProjectionMatrix),
				_projectionViewMatrix(projectionViewMatrix),
				_inverseProjectionViewMatrix(inverseProjectionViewMatrix),
				_ambientColor(ambientColor),
				_customData(customData),
				_fogColor0(fogColor0),
				_fogColor1(fogColor1),
				_clipDistance(clipDistance),
				_fogDistance(fogDistance),
				_tag(tag),
				_frame(frame)
			{
				for(size_t i = 0; i < FrustumPlaneCount; i += 1)
				{
					_frustumPlanes[i] = frustumPlanes ? frustumPlanes[i] : Vector4();
				}
			}

		public:
			static CameraSnapshot WithCamera(Camera *camera, const Rect &frame)
			{
				camera->PostUpdate();
				return WithCameraProjection(camera, frame, camera->GetProjectionMatrix());
			}

			static CameraSnapshot WithCamera(Camera *camera, const Rect &frame, const Matrix &projectionCorrection)
			{
				camera->PostUpdate();
				return WithCameraProjection(camera, frame, projectionCorrection * camera->GetProjectionMatrix());
			}

			Vector3 GetViewPosition() const { return Vector3(_inverseViewMatrix.m[12], _inverseViewMatrix.m[13], _inverseViewMatrix.m[14]); }
			const PositionType &GetRenderOrigin() const { return _renderOrigin; }
			uint64 GetSourceCameraUID() const { return _sourceCameraUID; }
			bool GetSortInstancable() const { return _sortInstancable; }
			const Matrix &GetViewMatrix() const { return _viewMatrix; }
			const Matrix &GetInverseViewMatrix() const { return _inverseViewMatrix; }
			const Matrix &GetProjectionMatrix() const { return _projectionMatrix; }
			const Matrix &GetInverseProjectionMatrix() const { return _inverseProjectionMatrix; }
			const Matrix &GetProjectionViewMatrix() const { return _projectionViewMatrix; }
			const Matrix &GetInverseProjectionViewMatrix() const { return _inverseProjectionViewMatrix; }
			const Vector4 *GetFrustumPlanes() const { return _frustumPlanes; }
			const Color &GetAmbientColor() const { return _ambientColor; }
			const Vector4 &GetCustomData() const { return _customData; }
			const Color &GetFogColor0() const { return _fogColor0; }
			const Color &GetFogColor1() const { return _fogColor1; }
			const Vector2 &GetClipDistance() const { return _clipDistance; }
			const Vector2 &GetFogDistance() const { return _fogDistance; }
			const int32 &GetTag() const { return _tag; }
			const Rect &GetFrame() const { return _frame; }
			CameraSnapshot WithFrame(const Rect &frame) const
			{
				CameraSnapshot snapshot(*this);
				snapshot._frame = frame;
				return snapshot;
			}

		private:
			static CameraSnapshot WithCameraProjection(Camera *camera, const Rect &frame, const Matrix &projectionMatrix)
			{
				Matrix viewMatrix = camera->GetViewMatrix();
				Matrix inverseViewMatrix = camera->GetInverseViewMatrix();
				Matrix inverseProjectionMatrix = projectionMatrix.GetInverse();
				Vector4 frustumPlanes[FrustumPlaneCount];
				camera->GetFrustumPlanes(frustumPlanes);
				return CameraSnapshot(camera->GetRenderOrigin(), camera->GetUID(), static_cast<bool>(camera->GetFlags() & Camera::Flags::SortInstancable), viewMatrix, inverseViewMatrix, projectionMatrix, inverseProjectionMatrix, projectionMatrix * viewMatrix, inverseViewMatrix * inverseProjectionMatrix, frustumPlanes, camera->GetAmbientColor(), camera->GetCustomData(), camera->GetFogColor0(), camera->GetFogColor1(), Vector2(camera->GetClipNear(), camera->GetClipFar()), Vector2(camera->GetFogNear(), camera->GetFogFar()), camera->GetTag(), frame);
			}

			PositionType _renderOrigin;
			uint64 _sourceCameraUID;
			bool _sortInstancable;
			Matrix _viewMatrix;
			Matrix _inverseViewMatrix;
			Matrix _projectionMatrix;
			Matrix _inverseProjectionMatrix;
			Matrix _projectionViewMatrix;
			Matrix _inverseProjectionViewMatrix;
			Vector4 _frustumPlanes[FrustumPlaneCount];
			Color _ambientColor;
			Vector4 _customData;
			Color _fogColor0;
			Color _fogColor1;
			Vector2 _clipDistance;
			Vector2 _fogDistance;
			int32 _tag;
			Rect _frame;
		};

		class DirectionalLight
		{
		public:
			DirectionalLight(const Vector3 &direction, const Vector4 &color) :
				_direction(direction),
				_padding(0.0f),
				_color(color)
			{}

		private:
			Vector3 _direction;
			float _padding;
			Vector4 _color;
		};

		class PointLight
		{
		public:
			PointLight(const Vector3 &position, float range, const Vector4 &color) :
				_position(position),
				_range(range),
				_color(color)
			{}

		private:
			Vector3 _position;
			float _range;
			Vector4 _color;
		};

		class SpotLight
		{
		public:
			SpotLight(const Vector3 &position, float range, const Vector3 &direction, float angle, const Vector4 &color) :
				_position(position),
				_range(range),
				_direction(direction),
				_angle(angle),
				_color(color)
			{}

		private:
			Vector3 _position;
			float _range;
			Vector3 _direction;
			float _angle;
			Vector4 _color;
		};

		class DrawItem
		{
		public:
			DrawItem(Drawable *sourceDrawable, const Drawable::DrawSnapshotBundle &drawSnapshot, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint64 sourceNodeUID, int32 renderPriority) :
				_sourceDrawable(sourceDrawable),
				_drawSnapshot(drawSnapshot),
				_modelMatrix(modelMatrix),
				_inverseModelMatrix(inverseModelMatrix),
				_sourceNodeUID(sourceNodeUID),
				_renderPriority(renderPriority),
				_meshInstancingHash(0)
			{
				sourceDrawable->GetMeshBufferSnapshot(_meshBuffers);
				_indirectDrawSnapshot = sourceDrawable->GetIndirectDrawSnapshot();
				HashCombine(_meshInstancingHash, GetMesh().GetVerticesCount());
				HashCombine(_meshInstancingHash, GetMesh().GetIndicesCount());
				HashCombine(_meshInstancingHash, GetMesh().GetPipelineHash());
				HashCombine(_meshInstancingHash, static_cast<uint32>(GetMesh().GetIndexType()));
				HashCombine(_meshInstancingHash, _meshBuffers.GetVertexBuffer());
				HashCombine(_meshInstancingHash, _meshBuffers.GetIndicesBuffer());
			}

			Drawable *GetSourceDrawableForPreparation() const { return _sourceDrawable; }
			const Mesh::DrawSnapshot &GetMesh() const { return _drawSnapshot.GetMesh(); }
			const Mesh::BufferSnapshot &GetMeshBuffers() const { return _meshBuffers; }
			const Material::DrawSnapshot &GetMaterial() const { return _drawSnapshot.GetMaterial(); }
			const Skeleton::DrawSnapshot &GetSkeleton() const { return _drawSnapshot.GetSkeleton(); }
			const Matrix &GetModelMatrix() const { return _modelMatrix; }
			const Matrix &GetInverseModelMatrix() const { return _inverseModelMatrix; }
			uint64 GetSourceNodeUID() const { return _sourceNodeUID; }
			int32 GetRenderPriority() const { return _renderPriority; }
			size_t GetMeshInstancingHash() const { return _meshInstancingHash; }
			uint64 GetMaterialSnapshotVersion() const { return _drawSnapshot.GetMaterialSnapshotVersion(); }
			const Drawable::IndirectDrawSnapshot &GetIndirectDrawSnapshot() const { return _indirectDrawSnapshot; }
			bool HasIndirectDraw() const { return _indirectDrawSnapshot.IsValid(); }

		private:
			Drawable *_sourceDrawable;
			Drawable::DrawSnapshotBundle _drawSnapshot;
			Mesh::BufferSnapshot _meshBuffers;
			Drawable::IndirectDrawSnapshot _indirectDrawSnapshot;
			Matrix _modelMatrix;
			Matrix _inverseModelMatrix;
			uint64 _sourceNodeUID;
			int32 _renderPriority;
			size_t _meshInstancingHash;
		};

		class Pass
		{
		public:
			Pass(const RenderPass::DrawSnapshot &drawSnapshot, const Material::DrawSnapshot *overrideMaterialSnapshot, uint64 overrideMaterialCacheIdentity, uint64 overrideMaterialSnapshotVersion) :
				_drawSnapshot(drawSnapshot),
				_hasOverrideMaterial(overrideMaterialSnapshot != nullptr),
				_overrideMaterialCacheIdentity(overrideMaterialSnapshot ? overrideMaterialCacheIdentity : 0),
				_overrideMaterialSnapshotVersion(overrideMaterialSnapshot ? overrideMaterialSnapshotVersion : 0)
			{
				if(_hasOverrideMaterial)
					_overrideMaterialSnapshot = *overrideMaterialSnapshot;
			}

			void AddDrawItemIndex(size_t drawItemIndex)
			{
				_drawItemIndices.push_back(drawItemIndex);
			}

			void SetCameraSnapshot(const CameraSnapshot &cameraSnapshot) { _cameraSnapshot = cameraSnapshot; }
			const CameraSnapshot &GetCameraSnapshot() const { return _cameraSnapshot; }
			void AddMultiviewCameraSnapshot(const CameraSnapshot &cameraSnapshot) { _multiviewCameraSnapshots.push_back(cameraSnapshot); }
			uint8 GetMultiviewCameraCount() const { return static_cast<uint8>(_multiviewCameraSnapshots.size()); }
			const std::vector<CameraSnapshot> &GetMultiviewCameraSnapshots() const { return _multiviewCameraSnapshots; }

			void AddLight(const Light *light)
			{
				if(light->GetType() == Light::Type::DirectionalLight)
				{
					_directionalLights.emplace_back(light->GetForward(), light->GetFinalColor());
				}
				else if(light->GetType() == Light::Type::PointLight)
				{
					_pointLights.emplace_back(Vector3(light->GetWorldPosition() - _cameraSnapshot.GetRenderOrigin()), light->GetRange(), light->GetFinalColor());
				}
				else if(light->GetType() == Light::Type::SpotLight)
				{
					_spotLights.emplace_back(Vector3(light->GetWorldPosition() - _cameraSnapshot.GetRenderOrigin()), light->GetRange(), light->GetForward(), light->GetAngleCos(), light->GetFinalColor());
				}
			}
			const std::vector<DirectionalLight> &GetDirectionalLights() const { return _directionalLights; }
			const std::vector<PointLight> &GetPointLights() const { return _pointLights; }
			const std::vector<SpotLight> &GetSpotLights() const { return _spotLights; }

			void AddAttachmentSnapshot(Object *snapshot)
			{
				RN_ASSERT(snapshot, "RenderFrame pass attachment snapshot mustn't be NULL");
				_attachmentSnapshots[snapshot->GetClass()] = snapshot;
			}

			template<class T>
			T *GetAttachmentSnapshot() const
			{
				auto iterator = _attachmentSnapshots.find(T::GetMetaClass());
				return iterator == _attachmentSnapshots.end() ? nullptr : iterator->second.Get()->template Downcast<T>();
			}

			Object *GetAttachmentSnapshot(MetaClass *meta) const
			{
				auto iterator = _attachmentSnapshots.find(meta);
				return iterator == _attachmentSnapshots.end() ? nullptr : iterator->second.Get();
			}

			template<class Callback>
			void EnumerateAttachmentSnapshots(Callback callback) const
			{
				for(const auto &attachmentSnapshot : _attachmentSnapshots)
				{
					callback(attachmentSnapshot.second.Get());
				}
			}

			void SetPassResourceBuffer(const String *name, GPUBuffer *buffer)
			{
				size_t nameHash = name->GetHash();
				if(buffer) _passResourceBuffers[nameHash] = buffer;
				else _passResourceBuffers.erase(nameHash);
			}

			GPUBuffer *GetPassResourceBuffer(const String *name) const
			{
				return GetPassResourceBuffer(name->GetHash());
			}

			GPUBuffer *GetPassResourceBuffer(size_t nameHash) const
			{
				auto iterator = _passResourceBuffers.find(nameHash);
				return iterator == _passResourceBuffers.end() ? nullptr : iterator->second.Get();
			}

			void SetPassResourceTexture(const String *name, Texture *texture)
			{
				size_t nameHash = name->GetHash();
				if(texture) _passResourceTextures[nameHash] = texture;
				else _passResourceTextures.erase(nameHash);
			}

			Texture *GetPassResourceTexture(const String *name) const
			{
				return GetPassResourceTexture(name->GetHash());
			}

			Texture *GetPassResourceTexture(size_t nameHash) const
			{
				auto iterator = _passResourceTextures.find(nameHash);
				return iterator == _passResourceTextures.end() ? nullptr : iterator->second.Get();
			}

			void SetPassUniform(const String *name, const void *data, size_t size)
			{
				size_t nameHash = name->GetHash();
				if(data && size > 0)
				{
					const uint8 *bytes = static_cast<const uint8 *>(data);
					_passUniforms[nameHash].assign(bytes, bytes + size);
				}
				else
				{
					_passUniforms.erase(nameHash);
				}
			}

			const std::vector<uint8> *GetPassUniform(const String *name) const
			{
				return GetPassUniform(name->GetHash());
			}

			const std::vector<uint8> *GetPassUniform(size_t nameHash) const
			{
				auto iterator = _passUniforms.find(nameHash);
				return iterator == _passUniforms.end() ? nullptr : &iterator->second;
			}

			void CopyPassUniform(size_t nameHash, void *destination, size_t size) const
			{
				std::memset(destination, 0, size);

				const std::vector<uint8> *passUniform = GetPassUniform(nameHash);
				RN_DEBUG_ASSERT(passUniform, "Missing pass uniform");
				if(passUniform)
				{
					RN_DEBUG_ASSERT(passUniform->size() <= size, "Pass uniform is larger than shader uniform descriptor");
					size_t copySize = std::min(passUniform->size(), size);
					std::memcpy(destination, passUniform->data(), copySize);
				}
			}

			const RenderPass::DrawSnapshot &GetDrawSnapshot() const { return _drawSnapshot; }
			const Material::DrawSnapshot *GetOverrideMaterialSnapshot() const { return _hasOverrideMaterial ? &_overrideMaterialSnapshot : nullptr; }
			uint64 GetOverrideMaterialCacheIdentity() const { return _overrideMaterialCacheIdentity; }
			uint64 GetOverrideMaterialSnapshotVersion() const { return _overrideMaterialSnapshotVersion; }
			const std::vector<size_t> &GetDrawItemIndices() const { return _drawItemIndices; }

		private:
			RenderPass::DrawSnapshot _drawSnapshot;
			bool _hasOverrideMaterial;
			uint64 _overrideMaterialCacheIdentity;
			uint64 _overrideMaterialSnapshotVersion;
			Material::DrawSnapshot _overrideMaterialSnapshot;
			std::vector<size_t> _drawItemIndices;
			CameraSnapshot _cameraSnapshot;
			std::vector<CameraSnapshot> _multiviewCameraSnapshots;
			std::vector<DirectionalLight> _directionalLights;
			std::vector<PointLight> _pointLights;
			std::vector<SpotLight> _spotLights;
			std::unordered_map<MetaClass *, StrongRef<Object>> _attachmentSnapshots;
			std::unordered_map<size_t, StrongRef<GPUBuffer>> _passResourceBuffers;
			std::unordered_map<size_t, StrongRef<Texture>> _passResourceTextures;
			std::unordered_map<size_t, std::vector<uint8>> _passUniforms;
		};

		static constexpr size_t InvalidPassIndex = static_cast<size_t>(-1);
		static constexpr size_t InvalidDrawItemIndex = static_cast<size_t>(-1);
		static constexpr uint64 InvalidSourceNodeUID = static_cast<uint64>(-1);

		void Clear()
		{
			_frameID = 0;
			_presentationStates.clear();
			_passes.clear();
			_drawItems.clear();
			_cameraStatistics.clear();
			_attachmentSnapshots.clear();
			_globalTextures.clear();
			_globalBuffers.clear();
#if RN_BUILD_DEBUG
			_globalTextureNames.clear();
			_globalBufferNames.clear();
#endif
		}

		uint64 GetFrameID() const { return _frameID; }
		
		void AddPresentationState(RenderFramePresentationState *state)
		{
			if(!state) return;

			for(const StrongRef<RenderFramePresentationState> &existingState : _presentationStates)
			{
				if(existingState.Get() == state) return;
			}

			_presentationStates.push_back(state);
		}
		
		bool BeginPresentationStatesOnRenderThread() const
		{
			size_t begunStateCount = 0;
			for(const StrongRef<RenderFramePresentationState> &state : _presentationStates)
			{
				if(!state->BeginFrameOnRenderThread())
				{
					CancelPresentationStatesOnRenderThread(begunStateCount);
					return false;
				}

				begunStateCount += 1;
			}

			return true;
		}

		void EndPresentationStatesOnRenderThread() const
		{
			for(const StrongRef<RenderFramePresentationState> &state : _presentationStates)
			{
				state->EndFrameOnRenderThread();
			}
		}

		void CancelPresentationStatesOnRenderThread() const
		{
			CancelPresentationStatesOnRenderThread(_presentationStates.size());
		}

		void AddAttachmentSnapshot(Object *snapshot)
		{
			RN_ASSERT(snapshot, "RenderFrame attachment snapshot mustn't be NULL");
			_attachmentSnapshots[snapshot->GetClass()] = snapshot;
		}

		template<class T>
		T *GetAttachmentSnapshot() const
		{
			auto iterator = _attachmentSnapshots.find(T::GetMetaClass());
			return iterator == _attachmentSnapshots.end() ? nullptr : iterator->second.Get()->template Downcast<T>();
		}

		Object *GetAttachmentSnapshot(MetaClass *meta) const
		{
			auto iterator = _attachmentSnapshots.find(meta);
			return iterator == _attachmentSnapshots.end() ? nullptr : iterator->second.Get();
		}

		void SetGlobalTexture(const String *name, Texture *texture)
		{
			size_t nameHash = name->GetHash();
#if RN_BUILD_DEBUG
			SetGlobalResourceName(_globalTextureNames, nameHash, name, texture != nullptr);
#endif
			if(texture) _globalTextures[nameHash] = texture;
			else _globalTextures.erase(nameHash);
		}

		Texture *GetGlobalTexture(const String *name) const
		{
			return GetGlobalTexture(name->GetHash());
		}

		Texture *GetGlobalTexture(size_t nameHash) const
		{
			auto iterator = _globalTextures.find(nameHash);
			return iterator == _globalTextures.end() ? nullptr : iterator->second.Get();
		}

		void SetGlobalBuffer(const String *name, GPUBuffer *buffer)
		{
			size_t nameHash = name->GetHash();
#if RN_BUILD_DEBUG
			SetGlobalResourceName(_globalBufferNames, nameHash, name, buffer != nullptr);
#endif
			if(buffer) _globalBuffers[nameHash] = buffer;
			else _globalBuffers.erase(nameHash);
		}

		GPUBuffer *GetGlobalBuffer(const String *name) const
		{
			return GetGlobalBuffer(name->GetHash());
		}

		GPUBuffer *GetGlobalBuffer(size_t nameHash) const
		{
			auto iterator = _globalBuffers.find(nameHash);
			return iterator == _globalBuffers.end() ? nullptr : iterator->second.Get();
		}

		size_t AddPass(const RenderPass::DrawSnapshot &drawSnapshot, const Material::DrawSnapshot *overrideMaterialSnapshot, uint64 overrideMaterialCacheIdentity, uint64 overrideMaterialSnapshotVersion)
		{
			_passes.emplace_back(drawSnapshot, overrideMaterialSnapshot, overrideMaterialCacheIdentity, overrideMaterialSnapshotVersion);
			return _passes.size() - 1;
		}

		size_t AddDrawItem(Drawable *sourceDrawable, const SceneNode *node, const Pass &pass)
		{
			const PositionType &renderOrigin = pass.GetCameraSnapshot().GetRenderOrigin();
			const Matrix modelMatrix = node ? node->GetWorldTransform(renderOrigin) : Matrix();
			const Matrix inverseModelMatrix = node ? node->GetInverseWorldTransform(renderOrigin) : Matrix();
			return AddDrawItem(sourceDrawable, modelMatrix, inverseModelMatrix, node ? node->GetUID() : InvalidSourceNodeUID, node ? node->GetRenderPriority() : SceneNode::RenderNormal);
		}

		size_t AddDrawItem(Drawable *sourceDrawable, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint64 sourceNodeUID = InvalidSourceNodeUID, int32 renderPriority = SceneNode::RenderNormal)
		{
			if(_drawItems.size() == _drawItems.capacity())
			{
				size_t capacity = _drawItems.capacity();
				_drawItems.reserve(capacity + (capacity > 0 ? capacity : RN_RENDERING_DRAW_ITEM_RESERVE_BLOCK_SIZE));
			}

			Drawable::DrawSnapshotBundle drawSnapshot = sourceDrawable->GetDrawSnapshotBundleForFrame(_frameID);
			_drawItems.emplace_back(sourceDrawable, drawSnapshot, modelMatrix, inverseModelMatrix, sourceNodeUID, renderPriority);
			return _drawItems.size() - 1;
		}

		void ReserveDrawItems(size_t count)
		{
			_drawItems.reserve(count);
		}

		const DrawItem &GetDrawItem(size_t index) const
		{
			RN_DEBUG_ASSERT(index < _drawItems.size(), "Invalid render frame draw item index");
			return _drawItems[index];
		}

		size_t GetDrawItemCount() const { return _drawItems.size(); }

		Pass &GetPass(size_t index)
		{
			RN_DEBUG_ASSERT(index < _passes.size(), "Invalid render frame pass index");
			return _passes[index];
		}

		const Pass &GetPass(size_t index) const
		{
			RN_DEBUG_ASSERT(index < _passes.size(), "Invalid render frame pass index");
			return _passes[index];
		}

		size_t GetPassCount() const { return _passes.size(); }

		size_t AddCameraStatistics()
		{
			_cameraStatistics.emplace_back();
			return _cameraStatistics.size() - 1;
		}

		CameraStatistics &GetCameraStatistics(size_t index)
		{
			RN_DEBUG_ASSERT(index < _cameraStatistics.size(), "Invalid render frame camera statistics index");
			return _cameraStatistics[index];
		}

		const std::vector<CameraStatistics> &GetCameraStatistics() const { return _cameraStatistics; }

	private:
		friend class Renderer;

		void SetFrameID(uint64 frameID) { _frameID = frameID; }
		void CancelPresentationStatesOnRenderThread(size_t count) const
		{
			if(count > _presentationStates.size())
				count = _presentationStates.size();

			for(size_t i = 0; i < count; i += 1)
			{
				_presentationStates[i]->CancelFrameOnRenderThread();
			}
		}

#if RN_BUILD_DEBUG
		void SetGlobalResourceName(std::unordered_map<size_t, StrongRef<String>> &names, size_t nameHash, const String *name, bool hasResource)
		{
			auto iterator = names.find(nameHash);
			RN_DEBUG_ASSERT(iterator == names.end() || iterator->second->IsEqual(name), "RenderFrame global resource names have a hash collision");

			if(hasResource) names[nameHash] = const_cast<String *>(name);
			else names.erase(nameHash);
		}
#endif

		uint64 _frameID = 0;
		std::vector<StrongRef<RenderFramePresentationState>> _presentationStates;
		std::deque<Pass> _passes;
		std::vector<DrawItem> _drawItems;
		std::vector<CameraStatistics> _cameraStatistics;
		std::unordered_map<MetaClass *, StrongRef<Object>> _attachmentSnapshots;
		std::unordered_map<size_t, StrongRef<Texture>> _globalTextures;
		std::unordered_map<size_t, StrongRef<GPUBuffer>> _globalBuffers;
#if RN_BUILD_DEBUG
		std::unordered_map<size_t, StrongRef<String>> _globalTextureNames;
		std::unordered_map<size_t, StrongRef<String>> _globalBufferNames;
#endif
	};

	class RenderPassDependencyCollector
	{
	public:
		void ReadsTexture(Texture *texture)
		{
			if(texture)
				_readTextures.push_back(texture);
		}

		const std::vector<Texture *> &GetReadTextures() const { return _readTextures; }

	private:
		std::vector<Texture *> _readTextures;
	};

	class RenderPassDependencyProvider : public Object
	{
	public:
		RNAPI virtual void CollectRenderPassDependencies(const RenderFrame::Pass &pass, RenderPassDependencyCollector &collector) const;

	protected:
		RNAPI RenderPassDependencyProvider();
		RNAPI ~RenderPassDependencyProvider() override;

		__RNDeclareMetaInternal(RenderPassDependencyProvider)
	};
} // namespace RN

#endif /* __RAYNE_RENDERFRAME_H__ */

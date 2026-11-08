//
//  RNDrawable.h
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_DRAWABLE_H_
#define __RAYNE_DRAWABLE_H_

#include "../Base/RNBase.h"
#include "../Math/RNMatrix.h"
#include "../Objects/RNObject.h"
#include "RNFramebuffer.h"
#include "RNMaterial.h"
#include "RNMesh.h"
#include "RNSkeleton.h"

namespace RN
{
	class RenderFrame;
	class Renderer;
	class RenderPass;
	class SceneNode;

	struct Drawable
	{
		RNAPI Drawable();
		Drawable(const Drawable &) = delete;
		Drawable &operator=(const Drawable &) = delete;
		RNAPI virtual ~Drawable();

		struct PipelineKey
		{
			size_t meshPipelineHash = 0;
			Framebuffer *framebuffer = nullptr;
			Shader *vertexShader = nullptr;
			Shader *fragmentShader = nullptr;
			Material::PipelineProperties materialProperties;
			RenderPass *renderPass = nullptr;
			uint64 renderPassSignature = 0;
			uint8 renderViewCount = 0;
			uint32 subpassIndex = 0;

			RNAPI bool operator==(const PipelineKey &other) const;
			bool operator!=(const PipelineKey &other) const { return !(*this == other); }
		};

		struct InstancingHashCache
		{
			bool TryGet(size_t meshHash, size_t textureSetHash, size_t &hash)
			{
				if(!_isValid || _meshHash != meshHash || _textureSetHash != textureSetHash)
				{
					_isValid = false;
					return false;
				}

				hash = _hash;
				return true;
			}

			void Store(size_t meshHash, size_t textureSetHash, size_t hash)
			{
				_meshHash = meshHash;
				_textureSetHash = textureSetHash;
				_hash = hash;
				_isValid = true;
			}

			void Invalidate() { _isValid = false; }

		private:
			size_t _meshHash = 0;
			size_t _textureSetHash = 0;
			size_t _hash = 0;
			bool _isValid = false;
		};

		struct MergedMaterialSnapshot
		{
			RNAPI void Update(const Material::DrawSnapshot &material, uint64 materialSnapshotVersion, Shader::UsageHint shaderHint, const Material::DrawSnapshot *overrideMaterialSnapshot, uint64 overrideMaterialSnapshotIdentity, uint64 overrideMaterialSnapshotVersion);
			Shader *GetVertexShader() const { return _vertexShader; }
			Shader *GetFragmentShader() const { return _fragmentShader; }
			const Array *GetTextures() const { return _textures; }
			size_t GetTextureSetHash() const { return _textureSetHash; }
			const Material::Properties &GetProperties() const { return _properties; }
			const Material::PipelineProperties &GetPipelineProperties() const { return _pipelineProperties; }

		private:
			bool _isValid = false;
			uint64 _materialSnapshotVersion = 0;
			uint64 _overrideSnapshotIdentity = 0;
			uint64 _overrideSnapshotVersion = 0;
			Shader::UsageHint _shaderHint = Shader::UsageHint::Default;
			Shader *_vertexShader = nullptr;
			Shader *_fragmentShader = nullptr;
			const Array *_textures = nullptr;
			size_t _textureSetHash = 0;
			Material::Properties _properties;
			Material::PipelineProperties _pipelineProperties;
		};

		enum class IndirectDrawType
		{
			Draw,
			DrawIndexed
		};

		class IndirectDrawSnapshot
		{
		public:
			IndirectDrawSnapshot() = default;
			IndirectDrawSnapshot(const IndirectDrawSnapshot &source, GPUBuffer::SnapshotReferences &bufferReferences) :
				_argumentBuffer(bufferReferences.CaptureBuffer(source._argumentBuffer.Get())),
				_argumentBufferOffset(source._argumentBufferOffset),
				_drawCount(source._drawCount),
				_stride(source._stride),
				_type(source._type)
			{}
			GPUBuffer *GetArgumentBuffer() const { return _argumentBuffer.Get(); }
			size_t GetArgumentBufferOffset() const { return _argumentBufferOffset; }
			uint32 GetDrawCount() const { return _drawCount; }
			size_t GetStride() const { return _stride; }
			IndirectDrawType GetType() const { return _type; }
			bool IsValid() const { return _argumentBuffer.Get() != nullptr; }

		private:
			friend struct Drawable;

			StrongRef<GPUBuffer> _argumentBuffer;
			size_t _argumentBufferOffset = 0;
			uint32 _drawCount = 1;
			size_t _stride = 0;
			IndirectDrawType _type = IndirectDrawType::Draw;
		};

		class PreparedDrawList;

		class DrawSnapshotBundle
		{
		public:
			const Mesh::DrawSnapshot &GetMesh() const { return *_mesh; }
			const Material::DrawSnapshot &GetMaterial() const { return *_material; }
			const Skeleton::DrawSnapshot &GetSkeleton() const { return *_skeleton; }
			uint64 GetMaterialSnapshotVersion() const { return _materialVersion; }

		private:
			friend struct Drawable;
			friend class PreparedDrawList;

			DrawSnapshotBundle(const Mesh::DrawSnapshot *mesh, const Material::DrawSnapshot *material, const Skeleton::DrawSnapshot *skeleton, uint64 materialVersion) :
				_mesh(mesh),
				_material(material),
				_skeleton(skeleton),
				_materialVersion(materialVersion)
			{}

			const Mesh::DrawSnapshot *_mesh;
			const Material::DrawSnapshot *_material;
			const Skeleton::DrawSnapshot *_skeleton;
			uint64 _materialVersion;
		};

		// One stable list per Entity LOD. Entries borrow the snapshots owned by their Drawables.
		class PreparedDrawList
		{
		public:
			PreparedDrawList() = default;
			PreparedDrawList(const PreparedDrawList &) = delete;
			PreparedDrawList &operator=(const PreparedDrawList &) = delete;
			RNAPI ~PreparedDrawList();

			void Reserve(size_t count) { _entries.reserve(count); }
			size_t GetCount() const { return _entries.size(); }
			RNAPI void Add(Drawable *drawable);
			RNAPI void Clear();

		private:
			friend struct Drawable;
			friend class RenderFrame;
			friend class Renderer;

			struct Entry
			{
				explicit Entry(Drawable *drawable) :
					drawable(drawable)
				{}

				Drawable *drawable;
				DrawSnapshotBundle snapshot{nullptr, nullptr, nullptr, 0};
				Mesh::BufferSnapshot meshBuffers;
				size_t meshInstancingHash = 0;
				const IndirectDrawSnapshot *indirectDraw = nullptr;
				bool hasStaticMeshBuffers = false;
			};

			const std::vector<Entry> &GetEntriesForFrame(uint64 frameID) const
			{
				if(_dirty.load(std::memory_order_acquire))
					Refresh(frameID);

				return _entries;
			}

			RNAPI void Refresh(uint64 frameID) const;

			mutable std::vector<Entry> _entries;
			mutable std::atomic<bool> _dirty{false};
		};

		RNAPI void SetSources(Mesh *mesh, Material *material, Skeleton *skeleton);
		RNAPI void SetIndirectDrawBuffer(GPUBuffer *argumentBuffer, IndirectDrawType type, size_t argumentBufferOffset = 0, uint32 drawCount = 1, size_t stride = 0);
		RNAPI void ClearIndirectDrawBuffer();
		RNAPI DrawSnapshotBundle GetDrawSnapshotBundleForFrame(uint64 frameID);
		const IndirectDrawSnapshot &GetIndirectDrawSnapshot() const { return _indirectDrawSnapshot; }

	private:
		friend class RenderFrame;
		friend class Material;
		friend class Mesh;
		friend class Skeleton;

		enum DrawSnapshotDirtyFlag : uint8
		{
			MeshSnapshotDirty = (1 << 0),
			MaterialSnapshotDirty = (1 << 1),
			SkeletonSnapshotDirty = (1 << 2),
			AllSnapshotsDirty = MeshSnapshotDirty | MaterialSnapshotDirty | SkeletonSnapshotDirty
		};

		static void MarkDrawSnapshotsDirty(const std::vector<Drawable *> &drawables, uint8 mask);
		void SetDrawSnapshotsDirty(uint8 mask);
		void UpdateDrawSnapshots(uint64 frameID);
		DrawSnapshotBundle MakeDrawSnapshotBundle() const;

		template<class Source>
		void SetSource(StrongRef<Source> &current, Source *source);

		template<class Source>
		static std::shared_ptr<const typename Source::DrawSnapshot> CaptureSourceSnapshot(Source *source)
		{
			if(source) return source->GetSharedDrawSnapshot();

			static const auto emptySnapshot = [] {
				auto snapshot = std::make_shared<typename Source::DrawSnapshot>();
				snapshot->Reset();
				return snapshot;
			}();
			return emptySnapshot;
		}

		static Lockable _preparedDrawLock;
		PreparedDrawList *_preparedDrawList = nullptr;
		// Writes hold _preparedDrawLock. Fallback submission reads the mask before taking the lock.
		std::atomic<uint8> _drawSnapshotDirtyMask{AllSnapshotsDirty};
		uint64 _materialSnapshotVersion = 0;
		std::shared_ptr<const Mesh::DrawSnapshot> _meshSnapshot;
		std::shared_ptr<const Material::DrawSnapshot> _materialSnapshot;
		std::shared_ptr<const Skeleton::DrawSnapshot> _skeletonSnapshot;

		// Keep source objects alive for snapshot refresh and source subscriptions.
		StrongRef<Mesh> _sourceMesh;
		StrongRef<Material> _sourceMaterial;
		StrongRef<Skeleton> _sourceSkeleton;
		IndirectDrawSnapshot _indirectDrawSnapshot;
	};
} // namespace RN


#endif /* __RAYNE_DRAWABLE_H_ */

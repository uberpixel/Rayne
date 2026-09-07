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
	class RenderPass;
	class SceneNode;

	struct Drawable
	{
		RNAPI Drawable();
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

		class DrawSnapshotBundle
		{
		public:
			const Mesh::DrawSnapshot &GetMesh() const { return *_mesh; }
			const Material::DrawSnapshot &GetMaterial() const { return *_material; }
			const Skeleton::DrawSnapshot &GetSkeleton() const { return *_skeleton; }
			uint64 GetMaterialSnapshotVersion() const { return _materialVersion; }

		private:
			friend struct Drawable;

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

		RNAPI void SetSources(Mesh *mesh, Material *material, Skeleton *skeleton);
		RNAPI void SetIndirectDrawBuffer(GPUBuffer *argumentBuffer, IndirectDrawType type, size_t argumentBufferOffset = 0, uint32 drawCount = 1, size_t stride = 0);
		RNAPI void ClearIndirectDrawBuffer();
		RNAPI void GetMeshBufferSnapshot(Mesh::BufferSnapshot &snapshot) const;
		RNAPI DrawSnapshotBundle GetDrawSnapshotBundleForFrame(uint64 frameID);
		const IndirectDrawSnapshot &GetIndirectDrawSnapshot() const { return _indirectDrawSnapshot; }

	private:
		static constexpr uint8 MeshSnapshotDirty = 1 << 0;
		static constexpr uint8 MaterialSnapshotDirty = 1 << 1;
		static constexpr uint8 SkeletonSnapshotDirty = 1 << 2;
		static constexpr uint8 AllSnapshotsDirty = MeshSnapshotDirty | MaterialSnapshotDirty | SkeletonSnapshotDirty;

		void UpdateDrawSnapshots(uint64 frameID);

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

		std::shared_ptr<const Mesh::DrawSnapshot> _meshSnapshot;
		std::shared_ptr<const Material::DrawSnapshot> _materialSnapshot;
		std::shared_ptr<const Skeleton::DrawSnapshot> _skeletonSnapshot;

		// Source objects are kept for snapshot refresh/version checks.
		StrongRef<Mesh> _sourceMesh;
		StrongRef<Material> _sourceMaterial;
		StrongRef<Skeleton> _sourceSkeleton;
		IndirectDrawSnapshot _indirectDrawSnapshot;

		uint64 _meshPipelineVersion = 0;
		uint64 _materialDrawSnapshotVersion = 0;
		uint64 _materialSnapshotVersion = 0;
		uint64 _skeletonDrawSnapshotVersion = 0;
		uint8 _drawSnapshotDirtyMask = AllSnapshotsDirty;
	};
} // namespace RN


#endif /* __RAYNE_DRAWABLE_H_ */

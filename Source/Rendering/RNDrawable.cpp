//
//  RNDrawable.cpp
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNDrawable.h"
#include "RNRenderer.h"

namespace RN
{
	Drawable::Drawable()
	{}

	Drawable::~Drawable() = default;

	bool Drawable::PipelineKey::operator==(const PipelineKey &other) const
	{
		return meshPipelineHash == other.meshPipelineHash &&
			framebuffer == other.framebuffer &&
			vertexShader == other.vertexShader &&
			fragmentShader == other.fragmentShader &&
			materialProperties == other.materialProperties &&
			renderPass == other.renderPass &&
			renderPassSignature == other.renderPassSignature &&
			renderViewCount == other.renderViewCount &&
			subpassIndex == other.subpassIndex;
	}

	void Drawable::MergedMaterialSnapshot::Update(const Material::DrawSnapshot &material, uint64 materialVersion, Shader::UsageHint hint, const Material::DrawSnapshot *overrideMaterialSnapshot, uint64 overrideMaterialSnapshotIdentity, uint64 overrideMaterialSnapshotVersion)
	{
		if(_isValid && _materialSnapshotVersion == materialVersion && _overrideSnapshotIdentity == overrideMaterialSnapshotIdentity && _overrideSnapshotVersion == overrideMaterialSnapshotVersion && _shaderHint == hint)
			return;

		_materialSnapshotVersion = materialVersion;
		_overrideSnapshotIdentity = overrideMaterialSnapshotIdentity;
		_overrideSnapshotVersion = overrideMaterialSnapshotVersion;
		_shaderHint = hint;
		_vertexShader = material.GetSelectedVertexShader(hint, overrideMaterialSnapshot);
		_fragmentShader = material.GetSelectedFragmentShader(hint, overrideMaterialSnapshot);
		const Array *overrideTextures = overrideMaterialSnapshot ? overrideMaterialSnapshot->GetTextures() : nullptr;
		_textures = (overrideTextures && overrideTextures->GetCount() > 0) ? overrideTextures : material.GetTextures();
		const size_t textureCount = _textures ? _textures->GetCount() : 0;
		_textureSetHash = 0;
		HashCombine(_textureSetHash, _textures != nullptr);
		HashCombine(_textureSetHash, textureCount);
		for(size_t i = 0; i < textureCount; i += 1)
		{
			HashCombine(_textureSetHash, _textures->GetObjectAtIndex(i));
		}
		material.GetMergedProperties(overrideMaterialSnapshot, _properties);
		material.GetMergedPipelineProperties(overrideMaterialSnapshot, _pipelineProperties);
		_isValid = true;
	}

	void Drawable::SetSources(Mesh *mesh, Material *material, Skeleton *skeleton)
	{
		if(_sourceMesh.Get() != mesh)
		{
			_sourceMesh = mesh;
			_drawSnapshotDirtyMask |= MeshSnapshotDirty;
		}
		if(_sourceMaterial.Get() != material)
		{
			_sourceMaterial = material;
			_drawSnapshotDirtyMask |= MaterialSnapshotDirty;
		}
		if(_sourceSkeleton.Get() != skeleton)
		{
			_sourceSkeleton = skeleton;
			_drawSnapshotDirtyMask |= SkeletonSnapshotDirty;
		}
	}

	void Drawable::SetIndirectDrawBuffer(GPUBuffer *argumentBuffer, IndirectDrawType type, size_t argumentBufferOffset, uint32 drawCount, size_t stride)
	{
		RN_ASSERT(!argumentBuffer || drawCount > 0, "Indirect draw count must be greater than zero");

		_indirectDrawSnapshot._argumentBuffer = argumentBuffer;
		_indirectDrawSnapshot._argumentBufferOffset = argumentBufferOffset;
		_indirectDrawSnapshot._drawCount = drawCount;
		_indirectDrawSnapshot._stride = stride;
		_indirectDrawSnapshot._type = type;
	}

	void Drawable::ClearIndirectDrawBuffer()
	{
		SetIndirectDrawBuffer(nullptr, IndirectDrawType::Draw, 0, 1, 0);
	}

	void Drawable::GetMeshBufferSnapshot(Mesh::BufferSnapshot &snapshot) const
	{
		Mesh *mesh = _sourceMesh.Get();
		if(mesh)
			mesh->GetBufferSnapshot(snapshot);
		else
			snapshot.Reset();
	}

	Drawable::DrawSnapshotBundle Drawable::GetDrawSnapshotBundleForFrame(uint64 frameID)
	{
		UpdateDrawSnapshots(frameID);

		RN_DEBUG_ASSERT(_meshSnapshot && _materialSnapshot && _skeletonSnapshot, "Drawable has no draw snapshots");

		return DrawSnapshotBundle(_meshSnapshot.get(), _materialSnapshot.get(), _skeletonSnapshot.get(), _materialSnapshotVersion);
	}

	void Drawable::UpdateDrawSnapshots(uint64 frameID)
	{
		Mesh *mesh = _sourceMesh.Get();
		uint64 meshPipelineVersion = mesh ? mesh->GetPipelineVersion() : 0;
		if(_meshPipelineVersion != meshPipelineVersion)
			_drawSnapshotDirtyMask |= MeshSnapshotDirty;

		Material *material = _sourceMaterial.Get();
		uint64 materialDrawSnapshotVersion = material ? material->GetDrawSnapshotVersion() : 0;
		if(_materialDrawSnapshotVersion != materialDrawSnapshotVersion)
			_drawSnapshotDirtyMask |= MaterialSnapshotDirty;

		Skeleton *skeleton = _sourceSkeleton.Get();
		uint64 skeletonDrawSnapshotVersion = skeleton ? skeleton->GetDrawSnapshotVersion() : 0;
		if(_skeletonDrawSnapshotVersion != skeletonDrawSnapshotVersion)
			_drawSnapshotDirtyMask |= SkeletonSnapshotDirty;

		if(_drawSnapshotDirtyMask == 0) return;

		std::shared_ptr<const void> retiredMesh;
		std::shared_ptr<const void> retiredMaterial;
		std::shared_ptr<const void> retiredSkeleton;

		if((_drawSnapshotDirtyMask & MeshSnapshotDirty) != 0)
		{
			retiredMesh = std::move(_meshSnapshot);
			_meshSnapshot = CaptureSourceSnapshot(mesh);
			_meshPipelineVersion = meshPipelineVersion;
		}

		if((_drawSnapshotDirtyMask & MaterialSnapshotDirty) != 0)
		{
			retiredMaterial = std::move(_materialSnapshot);
			_materialSnapshot = CaptureSourceSnapshot(material);
			_materialDrawSnapshotVersion = materialDrawSnapshotVersion;
			_materialSnapshotVersion += 1;
		}

		if((_drawSnapshotDirtyMask & SkeletonSnapshotDirty) != 0)
		{
			retiredSkeleton = std::move(_skeletonSnapshot);
			_skeletonSnapshot = CaptureSourceSnapshot(skeleton);
			_skeletonDrawSnapshotVersion = skeletonDrawSnapshotVersion;
		}

		_drawSnapshotDirtyMask = 0;
		if((retiredMesh || retiredMaterial || retiredSkeleton) && !Renderer::IsHeadless())
			Renderer::GetActiveRenderer()->RetireDrawSnapshots(frameID, std::move(retiredMesh), std::move(retiredMaterial), std::move(retiredSkeleton));
	}
} // namespace RN

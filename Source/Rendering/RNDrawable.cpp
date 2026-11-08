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
	Lockable Drawable::_preparedDrawLock;

	Drawable::Drawable()
	{}

	Drawable::~Drawable()
	{
		RN_DEBUG_ASSERT(!_preparedDrawList, "Drawable is still attached to a prepared draw list");
		SetSources(nullptr, nullptr, nullptr);
	}

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

	Drawable::PreparedDrawList::~PreparedDrawList()
	{
		Clear();
	}

	void Drawable::PreparedDrawList::Add(Drawable *drawable)
	{
		LockGuard<Lockable> lock(_preparedDrawLock);
		RN_DEBUG_ASSERT(!drawable->_preparedDrawList, "Drawable already has a prepared draw list");
		_entries.emplace_back(drawable);
		drawable->_preparedDrawList = this;
		_dirty.store(true, std::memory_order_release);
	}

	void Drawable::PreparedDrawList::Clear()
	{
		if(_entries.empty())
			return;

		LockGuard<Lockable> lock(_preparedDrawLock);
		for(const Entry &entry : _entries)
			entry.drawable->_preparedDrawList = nullptr;

		_entries.clear();
		_dirty.store(false, std::memory_order_relaxed);
	}

	void Drawable::PreparedDrawList::Refresh(uint64 frameID) const
	{
		LockGuard<Lockable> lock(_preparedDrawLock);
		for(Entry &entry : _entries)
		{
			Drawable *drawable = entry.drawable;
			drawable->UpdateDrawSnapshots(frameID);
			entry.snapshot = drawable->MakeDrawSnapshotBundle();
			entry.hasStaticMeshBuffers = entry.snapshot.GetMesh().GetStaticBuffersForDraw(entry.meshBuffers, entry.meshInstancingHash);
			entry.indirectDraw = drawable->_indirectDrawSnapshot.IsValid() ? &drawable->_indirectDrawSnapshot : nullptr;
		}

		_dirty.store(false, std::memory_order_release);
	}

	void Drawable::MarkDrawSnapshotsDirty(const std::vector<Drawable *> &drawables, uint8 mask)
	{
		if(drawables.empty())
			return;

		LockGuard<Lockable> lock(_preparedDrawLock);
		for(Drawable *drawable : drawables)
			drawable->SetDrawSnapshotsDirty(mask);
	}

	void Drawable::SetDrawSnapshotsDirty(uint8 mask)
	{
		const uint8 dirtyMask = _drawSnapshotDirtyMask.load(std::memory_order_relaxed);
		_drawSnapshotDirtyMask.store(dirtyMask | mask, std::memory_order_release);

		if(_preparedDrawList)
			_preparedDrawList->_dirty.store(true, std::memory_order_release);
	}

	template<class Source>
	void Drawable::SetSource(StrongRef<Source> &current, Source *source)
	{
		StrongRef<Source> replacement(source);

		if(source)
		{
			LockGuard<Source> lock(*source);
			source->_drawables.push_back(this);
		}

		if(Source *previous = current.Get())
		{
			LockGuard<Source> lock(*previous);
			auto &drawables = previous->_drawables;
			drawables.erase(std::find(drawables.begin(), drawables.end(), this));
		}

		current = std::move(replacement);
	}

	void Drawable::SetSources(Mesh *mesh, Material *material, Skeleton *skeleton)
	{
		const uint8 mask = (_sourceMesh.Get() != mesh ? MeshSnapshotDirty : 0) |
			(_sourceMaterial.Get() != material ? MaterialSnapshotDirty : 0) |
			(_sourceSkeleton.Get() != skeleton ? SkeletonSnapshotDirty : 0);
		if(mask == 0)
			return;

		// Rebinding and submission use the same thread. Invalidate first so a partial rebind can be retried.
		{
			LockGuard<Lockable> lock(_preparedDrawLock);
			SetDrawSnapshotsDirty(mask);
		}

		if(mask & MeshSnapshotDirty)
			SetSource(_sourceMesh, mesh);
		if(mask & MaterialSnapshotDirty)
			SetSource(_sourceMaterial, material);
		if(mask & SkeletonSnapshotDirty)
			SetSource(_sourceSkeleton, skeleton);
	}

	void Drawable::SetIndirectDrawBuffer(GPUBuffer *argumentBuffer, IndirectDrawType type, size_t argumentBufferOffset, uint32 drawCount, size_t stride)
	{
		RN_ASSERT(!argumentBuffer || drawCount > 0, "Indirect draw count must be greater than zero");

		StrongRef<GPUBuffer> previousBuffer(argumentBuffer);
		LockGuard<Lockable> lock(_preparedDrawLock);
		swap(_indirectDrawSnapshot._argumentBuffer, previousBuffer);
		_indirectDrawSnapshot._argumentBufferOffset = argumentBufferOffset;
		_indirectDrawSnapshot._drawCount = drawCount;
		_indirectDrawSnapshot._stride = stride;
		_indirectDrawSnapshot._type = type;

		if(_preparedDrawList)
			_preparedDrawList->_dirty.store(true, std::memory_order_release);
	}

	void Drawable::ClearIndirectDrawBuffer()
	{
		SetIndirectDrawBuffer(nullptr, IndirectDrawType::Draw, 0, 1, 0);
	}

	Drawable::DrawSnapshotBundle Drawable::GetDrawSnapshotBundleForFrame(uint64 frameID)
	{
		if(_drawSnapshotDirtyMask.load(std::memory_order_acquire) != 0)
		{
			LockGuard<Lockable> lock(_preparedDrawLock);
			UpdateDrawSnapshots(frameID);
		}

		return MakeDrawSnapshotBundle();
	}

	Drawable::DrawSnapshotBundle Drawable::MakeDrawSnapshotBundle() const
	{
		RN_DEBUG_ASSERT(_meshSnapshot && _materialSnapshot && _skeletonSnapshot, "Drawable has no draw snapshots");

		return DrawSnapshotBundle(_meshSnapshot.get(), _materialSnapshot.get(), _skeletonSnapshot.get(), _materialSnapshotVersion);
	}

	void Drawable::UpdateDrawSnapshots(uint64 frameID)
	{
		const uint8 dirtyMask = _drawSnapshotDirtyMask.load(std::memory_order_relaxed);
		if(dirtyMask == 0)
			return;

		auto meshSnapshot = (dirtyMask & MeshSnapshotDirty) ? CaptureSourceSnapshot(_sourceMesh.Get()) : nullptr;
		auto materialSnapshot = (dirtyMask & MaterialSnapshotDirty) ? CaptureSourceSnapshot(_sourceMaterial.Get()) : nullptr;
		auto skeletonSnapshot = (dirtyMask & SkeletonSnapshotDirty) ? CaptureSourceSnapshot(_sourceSkeleton.Get()) : nullptr;

		// Keep the current bundle owned until capture and retirement both succeed.
		if(((meshSnapshot && _meshSnapshot) || (materialSnapshot && _materialSnapshot) || (skeletonSnapshot && _skeletonSnapshot)) && !Renderer::IsHeadless())
		{
			Renderer::GetActiveRenderer()->RetireDrawSnapshots(frameID,
				meshSnapshot ? _meshSnapshot : nullptr,
				materialSnapshot ? _materialSnapshot : nullptr,
				skeletonSnapshot ? _skeletonSnapshot : nullptr);
		}

		if(meshSnapshot)
			_meshSnapshot = std::move(meshSnapshot);
		if(materialSnapshot)
		{
			_materialSnapshot = std::move(materialSnapshot);
			_materialSnapshotVersion += 1;
		}
		if(skeletonSnapshot)
			_skeletonSnapshot = std::move(skeletonSnapshot);

		_drawSnapshotDirtyMask.store(0, std::memory_order_relaxed);
	}
} // namespace RN

//
//  RNEntity.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEntity.h"

namespace RN
{
	RNDefineMeta(Entity, SceneNode)

	Entity::Entity() :
		_model(nullptr)
	{
		RN_PROFILE_SCOPE();
	}
	Entity::Entity(Model *model) :
		_model(nullptr)
	{
		RN_PROFILE_SCOPE();
		SetModel(model);
	}

	Entity::~Entity()
	{
		RN_PROFILE_SCOPE();
		SafeRelease(_model);
		ClearDrawables();
	}


	void Entity::ClearDrawables()
	{
		RN_PROFILE_SCOPE();
		if(Renderer::IsHeadless()) return;

		Renderer *renderer = Renderer::GetActiveRenderer();
#if RN_MODEL_LOD_DISABLED
		_renderableDrawables.Clear();
		for(Drawable *drawable : _drawables)
			renderer->DeleteDrawable(drawable);
#else
		_renderableDrawables.clear();
		for(const std::vector<Drawable *> &drawables : _drawables)
		{
			for(Drawable *drawable : drawables)
				renderer->DeleteDrawable(drawable);
		}
#endif

		_drawables.clear();
	}

	void Entity::SetModel(Model *model)
	{
		RN_PROFILE_SCOPE();
		SafeRelease(_model);
		_model = SafeRetain(model);

		ClearDrawables();

		if(_model)
		{
			if(!Renderer::IsHeadless())
			{
				Renderer *renderer = Renderer::GetActiveRenderer();
				size_t stages = _model->GetLODStageCount();

				for(size_t i = 0; i < stages; i++)
				{
					Model::LODStage *stage = _model->GetLODStage(i);
					size_t groups = stage->GetCount();

#if RN_MODEL_LOD_DISABLED //In this case there only ever is one stage
					_drawables.reserve(groups);
					for(size_t j = 0; j < groups; j++)
					{
						_drawables.push_back(renderer->CreateDrawable());
					}
#else
					_drawables.emplace_back();
					_renderableDrawables.emplace_back();

					std::vector<Drawable *> &drawables = _drawables.back();
					drawables.reserve(groups);
					for(size_t j = 0; j < groups; j++)
					{
						drawables.push_back(renderer->CreateDrawable());
					}
#endif
				}

				RefreshDrawableSources();
			}

			SetBoundingBox(model->GetBoundingBox());
		}
	}

	void Entity::SetDrawableRenderingEnabled(size_t index, bool enabled)
	{
		uint8 value = enabled ? 1 : 0;
		if(index >= _drawableRenderingEnabled.size())
		{
			_drawableRenderingEnabled.resize(index + 1, 1);
		}

		if(_drawableRenderingEnabled[index] == value)
			return;

		_drawableRenderingEnabled[index] = value;
		RebuildRenderableDrawables();
	}

	void Entity::Render(Renderer *renderer, Camera *camera) const
	{
		if(!_model)//RN_EXPECT_FALSE(!_model))
			return;

#if RN_MODEL_LOD_DISABLED
		const Drawable::PreparedDrawList &drawables = _renderableDrawables;
#else
		Camera *distanceCamera = camera->GetLODCamera();

		float lodDistance = GetWorldPosition().GetDistance(distanceCamera->GetWorldPosition());
		lodDistance /= distanceCamera->GetReferenceFar();

		const Model::LODStage *stage = _model->GetLODStageForDistance(lodDistance);

		size_t index = stage->_index;
		const Drawable::PreparedDrawList &drawables = _renderableDrawables[index];
#endif

		if(drawables.GetCount() > 0)
			renderer->SubmitDrawables(drawables, *this);
	}

	void Entity::RefreshDrawableSources()
	{
		RN_PROFILE_SCOPE();
		if(!_model) return;

#if RN_MODEL_LOD_DISABLED
		Model::LODStage *stage = _model->GetLODStage(0);
		size_t count = stage->GetCount();
		for(size_t i = 0; i < count; i += 1)
		{
			Drawable *drawable = _drawables[i];
			drawable->SetSources(stage->GetMeshAtIndex(i), stage->GetMaterialAtIndex(i), _model->_skeleton);
		}
#else
		size_t lodStageCount = _model->GetLODStageCount();
		for(size_t lodStage = 0; lodStage < lodStageCount; lodStage += 1)
		{
			Model::LODStage *stage = _model->GetLODStage(lodStage);
			const std::vector<Drawable *> &drawables = _drawables[lodStage];
			size_t count = stage->GetCount();
			for(size_t i = 0; i < count; i += 1)
			{
				Drawable *drawable = drawables[i];
				drawable->SetSources(stage->GetMeshAtIndex(i), stage->GetMaterialAtIndex(i), _model->_skeleton);
			}
		}
#endif
		RebuildRenderableDrawables();
	}

	bool Entity::CanRender(Renderer *renderer, Camera *camera) const
	{
		if(!_model) return false;

		return CanRenderUtil(renderer, camera);
	}

	bool Entity::IsDrawableRenderingEnabled(size_t index) const
	{
		return index >= _drawableRenderingEnabled.size() || _drawableRenderingEnabled[index];
	}

	void Entity::RebuildRenderableDrawables()
	{
#if RN_MODEL_LOD_DISABLED
		_renderableDrawables.Clear();
		_renderableDrawables.Reserve(_drawables.size());
		for(size_t i = 0; i < _drawables.size(); i += 1)
		{
			if(IsDrawableRenderingEnabled(i))
			{
				_renderableDrawables.Add(_drawables[i]);
			}
		}
#else
		for(size_t lodStage = 0; lodStage < _drawables.size(); lodStage += 1)
		{
			const std::vector<Drawable *> &drawables = _drawables[lodStage];
			Drawable::PreparedDrawList &renderableDrawables = _renderableDrawables[lodStage];
			renderableDrawables.Clear();
			renderableDrawables.Reserve(drawables.size());
			for(size_t i = 0; i < drawables.size(); i += 1)
			{
				if(IsDrawableRenderingEnabled(i))
				{
					renderableDrawables.Add(drawables[i]);
				}
			}
		}
#endif
	}
} // namespace RN

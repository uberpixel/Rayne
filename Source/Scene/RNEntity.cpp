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
	{}
	Entity::Entity(Model *model) :
		_model(nullptr)
	{
		SetModel(model);
	}

	Entity::~Entity()
	{
		SafeRelease(_model);
		ClearDrawables();
	}


	void Entity::ClearDrawables()
	{
		if(Renderer::IsHeadless()) return;
		
		Renderer *renderer = Renderer::GetActiveRenderer();
		for(auto &drawables : _drawables)
		{
			for(Drawable *drawable : drawables)
				renderer->DeleteDrawable(drawable);
		}

		_drawables.clear();
	}

	void Entity::SetModel(Model *model)
	{
		SafeRelease(_model);
		_model = SafeRetain(model);

		ClearDrawables();

		if(_model)
		{
			if(!Renderer::IsHeadless())
			{
				Renderer *renderer = Renderer::GetActiveRenderer();
				size_t stages = _model->GetLODStageCount();

				for(size_t i = 0; i < stages; i ++)
				{
					Model::LODStage *stage = _model->GetLODStage(i);
					size_t groups = stage->GetCount();

					_drawables.emplace_back(groups, nullptr);

					auto &drawables = _drawables.back();

					for(size_t j = 0; j < groups; j ++)
					{
						drawables[j] = 	renderer->CreateDrawable();
					}
				}
			}

			SetBoundingBox(model->GetBoundingBox());
		}
	}

	void Entity::Render(Renderer *renderer, Camera *camera) const
	{
		if(!RN_EXPECT_FALSE(_model))
			return;

		Camera *distanceCamera = camera->GetLODCamera();

		float lodDistance = GetWorldPosition().GetDistance(distanceCamera->GetWorldPosition());
		lodDistance /= distanceCamera->GetClipFar();

		Model::LODStage *stage = _model->GetLODStageForDistance(lodDistance);

		size_t index = stage->GetIndex();
		auto &drawables = _drawables[index];

		size_t count = stage->GetCount();

		for(size_t i = 0; i < count; i ++)
		{
			Material *material = stage->GetMaterialAtIndex(i);
			
			if(!material->GetSkipRendering())
			{
				Drawable *drawable = drawables[i];
				drawable->Update(stage->GetMeshAtIndex(i), material, _model->GetSkeleton(), this);
				renderer->SubmitDrawable(drawable);
			}
		}
	}

	void Entity::MakeDirty()
	{
		if(!_model) return;
		
		size_t lodStageCount = _model->GetLODStageCount();
		for(size_t lodStage = 0; lodStage < lodStageCount; lodStage += 1)
		{
			Model::LODStage *stage = _model->GetLODStage(lodStage);
			auto &drawables = _drawables[lodStage];

			size_t count = stage->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				Drawable *drawable = drawables[i];
				drawable->MakeDirty();
			}
		}
	}

	bool Entity::CanRender(Renderer *renderer, Camera *camera) const
	{
		if(!_model) return false;
		
		return CanRenderUtil(renderer, camera);
	}
}

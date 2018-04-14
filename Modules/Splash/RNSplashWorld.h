//
//  RNSplashWorld.h
//  Rayne-Splash
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPLASHWORLD_H_
#define __RAYNE_SPLASHWORLD_H_

#include "RNSplash.h"
#include "RNSplashBody.h"

namespace RN
{
	class SplashBody;

	struct SplashContactInfo
	{
		SceneNode *node;
		Vector3 position;
		Vector3 normal;
		float distance;
	};

	class SplashWorld : public SceneAttachment
	{
	public:
		SPAPI SplashWorld(const Vector3 &gravity = Vector3(0.0f, -9.81f, 0.0f));
		SPAPI ~SplashWorld();

		SPAPI void SetGravity(const Vector3 &gravity);

		SPAPI void Update(float delta) override;
		SPAPI void SetStepsPerFrame(uint16 stepCount);

		SPAPI void InsertBody(SplashBody *attachment);
		SPAPI void RemoveBody(SplashBody *attachment);

		SPAPI void SetPaused(bool paused);

		static SplashWorld *GetSharedInstance() { return _sharedInstance; }

	private:
		void StepSimulation(float delta);

		uint16 _stepsPerFrame;
		bool _paused;

		std::unordered_set<SplashBody *> _bodies;

		static SplashWorld *_sharedInstance;

		RNDeclareMetaAPI(SplashWorld, SPAPI)
	};
}


#endif /* __RAYNE_SPLASHWORLD_H_ */

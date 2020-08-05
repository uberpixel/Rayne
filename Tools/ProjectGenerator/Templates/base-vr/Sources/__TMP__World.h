//
//  __TMP__World.h
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

#ifndef ____TMP___WORLD_H_
#define ____TMP___WORLD_H_

#include <Rayne.h>

#include "RNPhysXWorld.h"

#include "__TMP__Types.h"
#include "__TMP__CameraManager.h"

namespace __TMP__
{
	class World : public RN::SceneBasic
	{
	public:
		static World *GetSharedInstance();
		static void Exit();

		World(RN::VRWindow *vrWindow);
		~World();

		RN::PhysXWorld *GetPhysicsWorld() const { return _physicsWorld; }
		RN::ShaderLibrary *GetShaderLibrary() const { return _shaderLibrary; }
		RN::VRCamera *GetVRCamera() const { return _cameraManager.GetVRCamera(); }
		RN::Camera *GetHeadCamera() const { return _cameraManager.GetHeadCamera(); }
		RN::Camera *GetPreviewCamera() const { return _cameraManager.GetPreviewCamera(); }
		CameraManager &GetCameraManager() { return _cameraManager; }

		RN::Model *AssignShader(RN::Model *model, Types::MaterialType materialType) const;
		RN::Model *MakeDeepCopy(RN::Model *model) const;
		
		void AddLevelNode(RN::SceneNode *node);
		void RemoveLevelNode(RN::SceneNode *node);
		void RemoveAllLevelNodes();
		
		bool GetIsDash() const { return _isDash; }
		
		void LoadLevel();

	protected:
		void WillBecomeActive() override;
		void DidBecomeActive() override;

		void WillUpdate(float delta) override;

		CameraManager _cameraManager;
		
		RN::Array *_levelNodes;

		RN::ShaderLibrary *_shaderLibrary;

		RN::VRWindow *_vrWindow;

		RN::PhysXWorld *_physicsWorld;
		
		bool _isPaused;
		bool _isDash;

		static World *_sharedInstance;
	};
}


#endif /* ____TMP___WORLD_H_ */

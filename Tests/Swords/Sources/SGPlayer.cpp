//
//  SGPlayer.cpp
//  Sword Game
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "SGPlayer.h"
#include "RNOculusCamera.h"
#include "SGWorld.h"
#include "RNBulletRigidBody.h"

namespace SG
{
	Player::Player(RN::SceneNode *camera, World *world, RN::BulletWorld *bulletWorld) :
	_camera(camera), _controller(nullptr), _throwKeyPressed(false), _bulletWorld(bulletWorld), _world(world), _bodyEntity(nullptr)
	{
		if(_camera->IsKindOfClass(RN::OculusCamera::GetMetaClass()))
		{
			_feetOffset = RN::Vector3(0.0f, -0.61f - 0.25f, 0.0f);
		}
		else
		{
			_feetOffset = RN::Vector3(0.0f, 0.61f + 0.18f, 0.0f);
		}

		SetWorldPosition(RN::Vector3(0.0, 0.0, 0.0) - _feetOffset);
		_camera->SetWorldPosition(GetWorldPosition() + _feetOffset);

		//Collision thingy
/*		_controller = new RN::BulletKinematicController(RN::BulletCapsuleShape::WithRadius(0.25f, 1.22f), 0.4f, RN::BulletCapsuleShape::WithRadius(0.22f, 1.22f));
		_controller->SetJumpSpeed(3.4f);
		AddAttachment(_controller);
		_bulletWorld->InsertCollisionObject(_controller);*/


		//Create the body entity
		RN::Mesh *boxMesh = RN::Mesh::WithColoredCube(RN::Vector3(0.28f, 0.5f, 0.08f), RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomColor());
		RN::MaterialDescriptor boxMaterialDescriptor;
		boxMaterialDescriptor.vertexShader = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::ShaderOptions::WithMesh(boxMesh));
		boxMaterialDescriptor.fragmentShader = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::ShaderOptions::WithMesh(boxMesh));
		RN::Material *boxMaterial = RN::Material::WithDescriptor(boxMaterialDescriptor);
		RN::Model *boxModel = new RN::Model(boxMesh, boxMaterial);
		_bodyEntity = new RN::Entity(boxModel);
		AddChild(_bodyEntity);

		//Create hand entities
		RN::Mesh *handMesh = RN::Mesh::WithColoredCube(RN::Vector3(0.05f, 0.05f, 0.05f), RN::Color::White());
		RN::MaterialDescriptor handMaterialDescriptor;
		handMaterialDescriptor.vertexShader = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::ShaderOptions::WithMesh(handMesh));
		handMaterialDescriptor.fragmentShader = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::ShaderOptions::WithMesh(handMesh));
		RN::Material *leftHandMaterial = RN::Material::WithDescriptor(handMaterialDescriptor);
		leftHandMaterial->SetDiffuseColor(RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomColor());
		RN::Model *leftHandModel = new RN::Model(handMesh, leftHandMaterial);
		_hand[0] = new RN::Entity(leftHandModel);
		AddChild(_hand[0]);

		RN::Material *rightHandMaterial = RN::Material::WithDescriptor(handMaterialDescriptor);
		rightHandMaterial->SetDiffuseColor(RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomColor());
		RN::Model *rightHandModel = new RN::Model(handMesh, rightHandMaterial);
		_hand[1] = new RN::Entity(rightHandModel);
		AddChild(_hand[1]);

		//Create the head entity
		RN::Mesh *headMesh = RN::Mesh::WithColoredCube(RN::Vector3(0.1f, 0.1f, 0.1f), RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomColor());
		RN::MaterialDescriptor headMaterialDescriptor;
		headMaterialDescriptor.vertexShader = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::ShaderOptions::WithMesh(headMesh));
		headMaterialDescriptor.fragmentShader = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::ShaderOptions::WithMesh(headMesh));
		RN::Material *headMaterial = RN::Material::WithDescriptor(headMaterialDescriptor);
		RN::Model *headModel = new RN::Model(headMesh, headMaterial);
		RN::Entity *headEntity = new RN::Entity(headModel);
		if(_camera->IsKindOfClass(RN::OculusCamera::GetMetaClass()))
		{
			RN::OculusCamera *oculusCamera = _camera->Downcast<RN::OculusCamera>();
			oculusCamera->GetHead()->AddChild(headEntity);
		}
	}
	
	Player::~Player()
	{
		//RN::SafeRelease(_controller);
	}
	
	void Player::Update(float delta)
	{
		RN::SceneNode::Update(delta);

		RN::Vector3 direction;
		RN::SceneNode *throwOrigin = nullptr;
		if(_camera->IsKindOfClass(RN::OculusCamera::GetMetaClass()))
		{
			RN::OculusCamera *oculusCamera = _camera->Downcast<RN::OculusCamera>();
			const RN::OculusTouchTrackingState &leftHandController = oculusCamera->GetTouchTrackingState(0);
			const RN::OculusTouchTrackingState &rightHandController = oculusCamera->GetTouchTrackingState(1);

			direction = RN::Vector3(leftHandController.thumbstick.x, 0.0f, -leftHandController.thumbstick.y);
			direction = leftHandController.rotation.GetRotatedVector(direction);
			direction.y = 0.0f;
			direction.Normalize();
			direction *= 0.025f;

			RN::Vector3 feetPosition = oculusCamera->GetHead()->GetPosition();
			feetPosition.y = 0.0f;
			_bodyOffset = -feetPosition;
			feetPosition += oculusCamera->GetWorldPosition(); //Breaks when rotating the oculus camera!
			RN::Vector3 targetPosition = feetPosition - _feetOffset + direction;
			direction = targetPosition - GetWorldPosition();

			if(rightHandController.indexTrigger > 0.5f)
				throwOrigin = _hand[1];
			if(leftHandController.indexTrigger > 0.5f)
				throwOrigin = _hand[0];
		}
		else
		{
			RN::InputManager *manager = RN::InputManager::GetSharedInstance();
			RN::Vector3 rotationX(-manager->GetMouseDelta().x*0.1f, 0.0f, 0.0f);
			Rotate(rotationX);

			RN::Vector3 rotationY(0.0f, -manager->GetMouseDelta().y*0.1f, 0.0f);
			rotationY += _cameraRotation;
			rotationY.y = std::max(-80.0f, std::min(65.0f, rotationY.y));
			_cameraRotation = rotationY;
			_camera->SetRotation(_cameraRotation);

			direction = RN::Vector3(manager->IsControlToggling(RNCSTR("D")) - manager->IsControlToggling(RNCSTR("A")), 0.0f, manager->IsControlToggling(RNCSTR("S")) - manager->IsControlToggling(RNCSTR("W")));
			direction = GetRotation().GetRotatedVector(direction);
			direction *= 0.025f;

			RN::Vector3 targetPosition = _camera->GetWorldPosition() + direction;
			direction = targetPosition - GetWorldPosition() - _feetOffset;

			if(manager->IsControlToggling(RNCSTR("F")))
			{
				throwOrigin = _camera;
			}
		}

		Translate(direction);

		//_controller->SetWalkDirection(direction);
		//_camera->SetWorldPosition(GetWorldPosition() + _feetOffset + _bodyOffset);

		if(_camera->IsKindOfClass(RN::OculusCamera::GetMetaClass()))
		{
			RN::OculusCamera *oculusCamera = _camera->Downcast<RN::OculusCamera>();
			const RN::OculusTouchTrackingState &leftHandController = oculusCamera->GetTouchTrackingState(0);
			const RN::OculusTouchTrackingState &rightHandController = oculusCamera->GetTouchTrackingState(1);

			//Hand placement
			_hand[0]->SetPosition(leftHandController.position + _bodyOffset + _feetOffset);
			_hand[0]->SetRotation(leftHandController.rotation);
			_hand[1]->SetPosition(rightHandController.position + _bodyOffset + _feetOffset);
			_hand[1]->SetRotation(rightHandController.rotation);

			//Body rotation
			RN::Vector3 bodyRotation = _bodyEntity->GetEulerAngle();
			if(direction.GetLength() > 0.01f)
			{
				bodyRotation = leftHandController.rotation.GetEulerAngle();
				bodyRotation.y = 0.0f;
				bodyRotation.z = 0.0f;
			}

			RN::Vector3 bodyForward = RN::Quaternion::WithEulerAngle(bodyRotation).GetRotatedVector(RN::Vector3(0.0f, 0.0f, -1.0f));
			bodyForward.y = 0.0f;
			bodyForward.Normalize();
			RN::Vector3 headForward = oculusCamera->GetHead()->GetForward();
			headForward.y = 0.0f;
			headForward.Normalize();
			if(bodyForward.GetDotProduct(headForward) < std::cosf(70.0f/180.0f*M_PI))
			{
				RN::Vector3 headRotation = oculusCamera->GetHead()->GetEulerAngle();
				float diff = headRotation.x - bodyRotation.x;
				if(diff > 180.0f) diff = headRotation.x - (360 + bodyRotation.x);
				if(diff < -180.0f) diff = 360 + headRotation.x - bodyRotation.x;
				if(diff > 0.0f)
				{
					headRotation.x -= 70.0f;
				}
				else
				{
					headRotation.x += 70.0f;
				}

				bodyRotation = headRotation;
				bodyRotation.y = 0.0f;
				bodyRotation.z = 0.0f;
			}

			_bodyEntity->SetRotation(bodyRotation);


/*			//Calculate neck position
			RN::Vector3 eyeVector(0.0f, 0.25f, -0.08f);
			RN::Vector3 headPosition = oculusCamera->GetHead()->GetPosition() + _bodyOffset + _feetOffset;
			RN::Vector3 neckPosition = headPosition - oculusCamera->GetHead()->GetRotation().GetRotatedVector(eyeVector);

			//Scale body from feet to head
			float heightFeetToNeck = neckPosition.y - _feetOffset.y;
			_bodyEntity->SetScale(RN::Vector3(1.0f, heightFeetToNeck, 1.0f));

			//Place body entity centered between ground and neck
			RN::Vector3 bodyPosition = neckPosition;
			bodyPosition.y = 0.0f;
			bodyPosition += _feetOffset;
			bodyPosition.y += heightFeetToNeck*0.5;
			_bodyEntity->SetPosition(bodyPosition);*/
		}

		if(throwOrigin)
		{
			if(!_throwKeyPressed)
			{
				ThrowBox(throwOrigin);
			}
			_throwKeyPressed = true;
		}
		else
		{
			_throwKeyPressed = false;
		}
	}

	void Player::DidUpdate(ChangeSet changeSet)
	{
		SceneNode::DidUpdate(changeSet);
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			_camera->SetWorldPosition(GetWorldPosition() + _feetOffset + _bodyOffset);

			if(_camera->IsKindOfClass(RN::OculusCamera::GetMetaClass()) && _bodyEntity)
			{
				RN::OculusCamera *oculusCamera = _camera->Downcast<RN::OculusCamera>();

				//Calculate neck position
				RN::Vector3 eyeVector(0.0f, 0.25f, -0.08f);
				RN::Vector3 headPosition = oculusCamera->GetHead()->GetPosition() + _bodyOffset + _feetOffset;
				RN::Vector3 neckPosition = headPosition - oculusCamera->GetHead()->GetRotation().GetRotatedVector(eyeVector);

				//Scale body from feet to head
				float heightFeetToNeck = neckPosition.y - _feetOffset.y;
				_bodyEntity->SetScale(RN::Vector3(1.0f, heightFeetToNeck, 1.0f));

				//Place body entity centered between ground and neck
				RN::Vector3 bodyPosition = neckPosition;
				bodyPosition.y = 0.0f;
				bodyPosition += _feetOffset;
				bodyPosition.y += heightFeetToNeck*0.5;
				_bodyEntity->SetPosition(bodyPosition);
			}
		}
	}

	void Player::ThrowBox(RN::SceneNode *origin)
	{
		RN::Mesh *boxMesh = RN::Mesh::WithColoredCube(RN::Vector3(0.1f, 0.1f, 0.1f), RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomColor());
		RN::MaterialDescriptor boxMaterialDescriptor;
		boxMaterialDescriptor.vertexShader = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::ShaderOptions::WithMesh(boxMesh));
		boxMaterialDescriptor.fragmentShader = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::ShaderOptions::WithMesh(boxMesh));
		RN::Material *boxMaterial = RN::Material::WithDescriptor(boxMaterialDescriptor);
		RN::Model *boxModel = new RN::Model(boxMesh, boxMaterial);
		RN::Entity *boxEntity = new RN::Entity(boxModel);
		boxEntity->SetWorldPosition(origin->GetWorldPosition());

		RN::BulletRigidBody *boxBody = RN::BulletRigidBody::WithShape(RN::BulletBoxShape::WithHalfExtents(RN::Vector3(0.1, 0.1, 0.1)), 1.0f);
		RN::BulletMaterial *boxBulletMaterial = new RN::BulletMaterial();
		boxBody->SetMaterial(boxBulletMaterial->Autorelease());
		boxEntity->AddAttachment(boxBody);
		_world->AddNode(boxEntity);
		_bulletWorld->InsertCollisionObject(boxBody);

		boxBody->ApplyImpulse(origin->GetForward()*5.0f);
	}
}

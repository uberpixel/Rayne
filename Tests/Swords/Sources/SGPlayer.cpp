//
//  SGPlayer.cpp
//  Sword Game
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "SGPlayer.h"
#include "RNOpenVRCamera.h"
#include "SGWorld.h"
#include "RNBulletRigidBody.h"

namespace SG
{
	Player::Player(RN::VRCamera *camera, World *world, RN::BulletWorld *bulletWorld) :
	_camera(camera), _controller(nullptr), _throwKeyPressed(false), _bulletWorld(bulletWorld), _world(world), _bodyEntity(nullptr)
	{
		_feetOffset = RN::Vector3(0.0f, -0.61f - 0.25f, 0.0f);

		//Collision thingy
/*		_controller = new RN::BulletKinematicController(RN::BulletCapsuleShape::WithRadius(0.25f, 1.22f), 0.4f, RN::BulletCapsuleShape::WithRadius(0.22f, 1.22f));
		_controller->SetJumpSpeed(3.4f);
		AddAttachment(_controller);
		_bulletWorld->InsertCollisionObject(_controller);*/


		//Create the body entity
		RN::Mesh *boxMesh = RN::Mesh::WithColoredCube(RN::Vector3(0.28f, 0.5f, 0.08f), RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomColor());
		RN::MaterialDescriptor boxMaterialDescriptor;
		boxMaterialDescriptor.vertexShader[0] = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::Shader::Options::WithMesh(boxMesh));
		boxMaterialDescriptor.fragmentShader[0] = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::Shader::Options::WithMesh(boxMesh));
		RN::Material *boxMaterial = RN::Material::WithDescriptor(boxMaterialDescriptor);
		RN::Model *boxModel = new RN::Model(boxMesh, boxMaterial);
		_bodyEntity = new RN::Entity(boxModel);
		AddChild(_bodyEntity);

		//Create hand entities
		RN::Mesh *handMesh = RN::Mesh::WithColoredCube(RN::Vector3(0.05f, 0.05f, 0.05f), RN::Color::White());
		RN::MaterialDescriptor handMaterialDescriptor;
		handMaterialDescriptor.vertexShader[0] = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::Shader::Options::WithMesh(handMesh));
		handMaterialDescriptor.fragmentShader[0] = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::Shader::Options::WithMesh(handMesh));
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
		headMaterialDescriptor.vertexShader[0] = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::Shader::Options::WithMesh(headMesh));
		headMaterialDescriptor.fragmentShader[0] = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::Shader::Options::WithMesh(headMesh));
		RN::Material *headMaterial = RN::Material::WithDescriptor(headMaterialDescriptor);
		RN::Model *headModel = new RN::Model(headMesh, headMaterial);
		RN::Entity *headEntity = new RN::Entity(headModel);
		_camera->GetHead()->AddChild(headEntity);

		SetWorldPosition(RN::Vector3(0.0, 0.0, 0.0) - _feetOffset);
		_camera->SetWorldPosition(GetWorldPosition() + _feetOffset);
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
		const RN::VRControllerTrackingState &leftHandController = _camera->GetControllerTrackingState(0);
		const RN::VRControllerTrackingState &rightHandController = _camera->GetControllerTrackingState(1);

		direction = RN::Vector3(leftHandController.thumbstick.x, 0.0f, -leftHandController.thumbstick.y);
		direction = leftHandController.rotation.GetRotatedVector(direction);
		direction.y = 0.0f;
		direction.Normalize();
		direction *= 0.025f;

		RN::Vector3 feetPosition = _camera->GetHead()->GetPosition();
		feetPosition.y = 0.0f;
		_bodyOffset = -feetPosition;
		feetPosition += _camera->GetWorldPosition(); //Breaks when rotating the oculus camera!
		RN::Vector3 targetPosition = feetPosition - _feetOffset + direction;
		direction = targetPosition - GetWorldPosition();

		if(rightHandController.indexTrigger > 0.5f)
			throwOrigin = _hand[1];
		if(leftHandController.indexTrigger > 0.5f)
			throwOrigin = _hand[0];

		Translate(direction);

		//_controller->SetWalkDirection(direction);
		//_camera->SetWorldPosition(GetWorldPosition() + _feetOffset + _bodyOffset);

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
		RN::Vector3 headForward = _camera->GetHead()->GetForward();
		headForward.y = 0.0f;
		headForward.Normalize();
		if(bodyForward.GetDotProduct(headForward) < std::cosf(70.0f/180.0f*M_PI))
		{
			RN::Vector3 headRotation = _camera->GetHead()->GetEulerAngle();
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

			//Calculate neck position
			RN::Vector3 eyeVector(0.0f, 0.25f, -0.08f);
			RN::Vector3 headPosition = _camera->GetHead()->GetPosition() + _bodyOffset + _feetOffset;
			RN::Vector3 neckPosition = headPosition - _camera->GetHead()->GetRotation().GetRotatedVector(eyeVector);

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

	void Player::ThrowBox(RN::SceneNode *origin)
	{
		RN::Mesh *boxMesh = RN::Mesh::WithColoredCube(RN::Vector3(0.1f, 0.1f, 0.1f), RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomColor());
		RN::MaterialDescriptor boxMaterialDescriptor;
		boxMaterialDescriptor.vertexShader[0] = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::Shader::Options::WithMesh(boxMesh));
		boxMaterialDescriptor.fragmentShader[0] = RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::Shader::Options::WithMesh(boxMesh));
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

#include "stdafx.h"
#include "AnimatedPhysics.h"

#include "SkeletonBody.h"
#include "math.h"
#include "PsiUtils.h"

namespace psi {

	AnimatedPhysics::AnimatedPhysics(spine::SkeletonRenderer* renderInstance, b2World* physicsWorld, const b2Vec2& renderToBodyScale) :
		renderInstance(renderInstance),
		physicsWorld(physicsWorld),
		renderToBodyScale(renderToBodyScale)
	{
		//this->renderToBodyScale *= 0.5f;
	}

	AnimatedPhysics::~AnimatedPhysics()
	{
		/// <TODO> : decide if we should just let the b2Bodies be in the world
		//for (b2Body* body : b2Bodies) {
		//	physicsWorld->DestroyBody(body);
		//}
	}

	void AnimatedPhysics::insertBody(b2Body* const & body, spBone* const & bone)
	{
		BodyData* data = new BodyData();
		data->bone = bone;
		body->SetUserData(data);
		b2Bodies.insert(body);
	}
	void AnimatedPhysics::destroyBody(b2Body* const & body)
	{
		// delete BodyData
		delete static_cast<BodyData*>(body->GetUserData());

		// get body's joints to remove from our set
		b2JointEdge* jointEdge = body->GetJointList();
		while (jointEdge) {
			b2Joints.erase(jointEdge->joint);
			jointEdge = jointEdge->next;
		}

		// remove body from our set
		b2Bodies.erase(body);

		// destroy body & joints
		physicsWorld->DestroyBody(body);
	}
	void AnimatedPhysics::insertJoint(b2Joint * const & joint)
	{
		b2Joints.insert(joint);
	}
	void AnimatedPhysics::destroyJoints()
	{
		for (b2Joint* joint : b2Joints) {
			physicsWorld->DestroyJoint(joint);
		}
	}

	void AnimatedPhysics::getBoneScreenTransform(const spBone* const bone, const cocos2d::Mat4& renderTransform, const float& renderRotation, cocos2d::Vec3& outWorldPosition, float& outWorldRotation) const
	{
		outWorldRotation = atan2(bone->c, bone->a) + M_PI_2 - renderRotation;

		/// <TODO> : this should of course be optimized!!!!
		outWorldPosition.set(bone->worldX, bone->worldY, 0);
		renderTransform.transformPoint(&outWorldPosition); // slow
		outWorldPosition.x *= renderToBodyScale.x;
		outWorldPosition.y *= renderToBodyScale.y;
	}

	void AnimatedPhysics::teleportBodiesToCurrentPose()
	{
		auto renderTransform = renderInstance->getNodeToWorldTransform();
		// this could be optimized, but probably not required the hassle
		cocos2d::Quaternion renderQuaternion;
		renderTransform.getRotation(&renderQuaternion);
		float renderRotation = renderQuaternion.toAxisAngle(&cocos2d::Vec3(0, 0, 1));

		for (b2Body* body : b2Bodies)
		{
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			data->previousImpulse.SetZero();
			data->previousTorque = 0;

			spBone* bone = data->bone;
			cocos2d::Vec3 screenPosition;
			float screenRotation;
			getBoneScreenTransform(bone, renderTransform, renderRotation, screenPosition, screenRotation);

			body->SetTransform(b2Vec2(screenPosition.x, screenPosition.y), screenRotation );
		}
	}

	// inverse function of "teleportBodiesToCurrentPose"
	void AnimatedPhysics::matchPoseToBodies()
	{
		for (b2Body* body : b2Bodies)
		{
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			spBone* bone = data->bone;
			float newR = spBone_worldToLocalRotation(bone->parent, RAD_TO_DEGf(body->GetAngle())) - 90; ;
			CCLOG("Bone %s o=%f n=%f", bone->data->name, bone->rotation, newR);
			bone->rotation = newR;
			//spBone_updateWorldTransform(bone);
		}
	}

	void AnimatedPhysics::impulseBodiesToCurrentPose()
	{
		auto renderTransform = renderInstance->getNodeToWorldTransform();
		// this could be optimized, but probably not required the hassle
		cocos2d::Quaternion renderQuaternion;
		renderTransform.getRotation(&renderQuaternion);
		float renderRotation = renderQuaternion.toAxisAngle(&cocos2d::Vec3(0, 0, 1));

		float timeStep = 1.f / 60;
		float invTimeStep = 1.f / timeStep;

		for (b2Body* body : b2Bodies)
		{
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			spBone* bone = data->bone;
			cocos2d::Vec3 screenPosition;
			float screenRotation;
			getBoneScreenTransform(bone, renderTransform, renderRotation, screenPosition, screenRotation);
			const b2Transform& prevTransform = body->GetTransform();

			// hack-in linear impulse
			// 0.95f are there in order NOT to accumulate error in "previousImpulse"
			b2Vec2 newImpulse{ b2Vec2(screenPosition.x, screenPosition.y) - body->GetPosition() };
			newImpulse *= invTimeStep;
			const_cast<b2Vec2&>(body->GetLinearVelocity()) += 0.95f * (newImpulse - data->previousImpulse); // doesn't require "friend class"
			data->previousImpulse = 0.95f * newImpulse;
			bone->data->name;

			// hack-in angular impulse
			float newTorque = screenRotation - body->GetAngle();
			newTorque *= invTimeStep;
			body->m_angularVelocity += newTorque - data->previousTorque; // not too bad to actually call "SetAngularVelocity", but better make body return "const float&"
			data->previousTorque = 0.95f * newTorque;
		}
	}
}
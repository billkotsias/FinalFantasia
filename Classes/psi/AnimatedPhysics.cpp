#include "stdafx.h"
#include "AnimatedPhysics.h"

#include "SkeletonBody.h"
#include "math.h"
#include "PsiUtils.h"

namespace psi {

	AnimatedPhysics::AnimatedPhysics(spine::SkeletonRenderer* renderInstance, b2World* physicsWorld, const float& renderToBodyScale) :
		renderInstance(renderInstance),
		physicsWorld(physicsWorld),
		renderToBodyScale(renderToBodyScale)
	{
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
		b2Bodies.push_back(body);
	}
	void AnimatedPhysics::destroyBody(b2Body* const & body)
	{
		// delete BodyData
		delete static_cast<BodyData*>(body->GetUserData());

		// remove body from our set
		// consider using lower_bound() in the future, in case pointers are REALLY sorted in vector!
		deque<b2Body*>::iterator it = std::find(b2Bodies.begin(), b2Bodies.end(), body);
#ifndef _DEBUG
		if (it != b2Bodies.end())
#endif // !_DEBUG
			b2Bodies.erase( it );

		// destroy body & joints
		physicsWorld->DestroyBody(body);
	}
	void AnimatedPhysics::destroyJoints()
	{
		for (b2Body* body : b2Bodies)
		{
			b2JointEdge* jointEdge = body->GetJointList();
			while (jointEdge) {
				physicsWorld->DestroyJoint(jointEdge->joint);
				jointEdge = jointEdge->next;
			}
		}
	}

	void AnimatedPhysics::getBoneRelativeTransform(const spBone* const bone, const cocos2d::AffineTransform& renderTransform, const float& renderRotation, cocos2d::Vec3& outWorldPosition, float& outWorldRotation) const
	{
		outWorldRotation = atan2(bone->c, bone->a) + M_PI_2 - renderRotation;
		// no offsets
		outWorldPosition.x = bone->worldX * renderTransform.a + bone->worldY * renderTransform.c;
		outWorldPosition.x *= renderToBodyScale;
		outWorldPosition.y = bone->worldX * renderTransform.b + bone->worldY * renderTransform.d;
		outWorldPosition.y *= renderToBodyScale;
	}

	void AnimatedPhysics::getBoneScreenTransform(const spBone* const bone, const cocos2d::AffineTransform& renderTransform, const float& renderRotation, cocos2d::Vec3& outWorldPosition, float& outWorldRotation) const
	{
		outWorldRotation = atan2(bone->c, bone->a) + M_PI_2 - renderRotation;

		// new, super-fast way
		outWorldPosition.x = bone->worldX * renderTransform.a + bone->worldY * renderTransform.c + renderTransform.tx;
		outWorldPosition.x *= renderToBodyScale;

		outWorldPosition.y = bone->worldX * renderTransform.b + bone->worldY * renderTransform.d + renderTransform.ty;
		outWorldPosition.y *= renderToBodyScale;
		/// old, super-slow way
		//outWorldPosition.set(bone->worldX, bone->worldY, 0);
		//renderTransform.transformPoint(&outWorldPosition); // slow
		//outWorldPosition.x *= renderToBodyScale.x;
		//outWorldPosition.y *= renderToBodyScale.y;
	}

	void AnimatedPhysics::setBonesToSetupPose()
	{
		for (b2Body* body : b2Bodies)
		{
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			spBone* bone = data->bone;
			bone->x = bone->data->x;
			bone->y = bone->data->y;
		}
	}

	void AnimatedPhysics::teleportBodiesToCurrentPose()
	{
		//auto renderTransform = renderInstance->getNodeToWorldTransform(); // old, slow, "buggy"
		auto renderTransform = renderInstance->getNodeToWorldAffineTransform();
		float renderRotation = atan2(renderTransform.c, renderTransform.a);

		for (b2Body* body : b2Bodies)
		{
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			data->previousImpulse.SetZero();
			data->previousTorque = 0;

			spBone* bone = data->bone;
			cocos2d::Vec3 screenPosition;
			float screenRotation;
			getBoneScreenTransform(bone, renderTransform, renderRotation, screenPosition, screenRotation);
			//if (string("left hand") == bone->data->name) CCLOG("%s %f %f", bone->data->name, RAD_TO_DEGf(screenRotation), RAD_TO_DEGf(renderRotation));

			body->SetTransform(b2Vec2(screenPosition.x, screenPosition.y), screenRotation );
			/// <TODO> : pass-in "alpha" value in SkeletonBody (FSL?)
			float invComeback = 0.95f; // 0 = immediately, 1 = never
			const_cast<b2Vec2&>(body->GetLinearVelocity()) *= invComeback; // doesn't require "friend class"
			body->m_angularVelocity *= invComeback;
		}
	}

	// inverse function of "teleportBodiesToCurrentPose"
	void AnimatedPhysics::matchPoseToBodies()
	{
		// essential to traverse bones hierarchically, update parent transform before childrens'
		deque<b2Body*>::iterator itBodies = b2Bodies.begin();
		if (itBodies == b2Bodies.end()) return;

		auto renderInverseTransform = renderInstance->getWorldToNodeTransform();
		auto renderTransform = renderInstance->getNodeToWorldAffineTransform();
		float renderRotation = atan2(renderTransform.c, renderTransform.a);

		{	// root bone -> put root body's position to 'renderInstance'
			b2Body* body = *(itBodies++);
			cocos2d::Vec3 pos(body->GetPosition().x, body->GetPosition().y, 0);
			pos.x /= renderToBodyScale;
			pos.y /= renderToBodyScale;
			renderInstance->getParent()->getWorldToNodeTransform().transformPoint(&pos);
			renderInstance->setPosition(pos.x, pos.y);
			CCLOG("RI %s o=%f,%f", static_cast<BodyData*>(body->GetUserData())->bone->data->name, renderInstance->getPosition().x, renderInstance->getPosition().y);
		}

		for (; itBodies != b2Bodies.end(); ++itBodies)
		{
			b2Body* body = *itBodies;
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			spBone* bone = data->bone;

			// rotation
			bone->rotation = spBone_worldToLocalRotation(bone->parent, RAD_TO_DEGf(body->GetAngle() + renderRotation - M_PI_2));

			// position is a lot more hassle-full
			/// <TODO> : slow!
			/// <REQUIRED IF WE HAVE JOINTS?>
			cocos2d::Vec3 pos(body->GetPosition().x, body->GetPosition().y, 0);
			pos.x /= renderToBodyScale;
			pos.y /= renderToBodyScale;
			renderInverseTransform.transformPoint(&pos);
			spBone_worldToLocal(bone->parent, pos.x, pos.y, &pos.x, &pos.y);
			//CCLOG("Bone %s o=%f,%f | n=%f,%f", bone->data->name, bone->x, bone->y, pos.x, pos.y);
			bone->x = pos.x;
			bone->y = pos.y;

			// update bone
			spBone_updateWorldTransform(bone);
		}
	}

	void AnimatedPhysics::impulseBodiesToCurrentPose(float timeStep)
	{
		auto renderTransform = renderInstance->getNodeToWorldAffineTransform();
		float renderRotation = atan2(renderTransform.c, renderTransform.a);

		float invTimeStep = 1.f / timeStep;

		for (b2Body* body : b2Bodies)
		{
			body->SetAwake(true);
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			spBone* bone = data->bone;
			cocos2d::Vec3 screenPosition;
			float screenRotation;
			getBoneScreenTransform(bone, renderTransform, renderRotation, screenPosition, screenRotation);
			const b2Transform& prevTransform = body->GetTransform();

			// hack-in linear impulse
			/// <TODO> : make this user-defined
			// how much the skeleton reacts to external forces
			float damping = .1f; // inverted damping, that is
			//b2Vec2 newImpulse{ b2Vec2(screenPosition.x, screenPosition.y) - body->GetPosition() };
			//newImpulse *= invTimeStep;
			//b2Vec2& speedRef = const_cast<b2Vec2&>(body->GetLinearVelocity()); // doesn't require "friend class"
			//speedRef -= data->previousImpulse;
			//speedRef *= damping;
			//speedRef += (data->previousImpulse = newImpulse);

			// hack-in angular impulse
			float newTorque = screenRotation - body->GetAngle();
			while (newTorque > M_PI) newTorque -= M_2PI;
			while (newTorque < -M_PI) newTorque += M_2PI;
			newTorque *= invTimeStep;
			body->m_angularVelocity -= data->previousTorque; // not too bad to actually call "SetAngularVelocity", but better make body return "const float&"
			body->m_angularVelocity *= damping;
			body->m_angularVelocity += (data->previousTorque = newTorque);
		}
	}
}
#include "stdafx.h"
#include "AnimatedPhysics.h"

#include "SkeletonBody.h"
#include "math.h"
#include "PsiUtils.h"

namespace psi {

	AnimatedPhysics::AnimatedPhysics(spine::SkeletonRenderer* renderInstance, b2World* physicsWorld, const float& renderToBodyScale,
		const bool& hasJoints, const float& timeStep) :
		renderInstance(renderInstance),
		physicsWorld(physicsWorld),
		renderToBodyScale(renderToBodyScale),
		hasJoints(hasJoints)
	{
		setTimeStep(timeStep);
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

	// only issue on initialization
	void AnimatedPhysics::teleportBodiesToCurrentPose()
	{
		//auto renderTransform = renderInstance->getNodeToWorldTransform(); // old, slow, "buggy"
		auto renderTransform = renderInstance->getNodeToWorldAffineTransform();
		float renderRotation = atan2(renderTransform.c, renderTransform.a);

		for (b2Body* body : b2Bodies)
		{
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			spBone* bone = data->bone;
			b2Vec2 screenPosition = boneToBodyPosition(bone, renderTransform);
			float screenRotation = boneToBodyRotation(bone, renderRotation);

			body->SetTransform(screenPosition, screenRotation);
			/// <TODO> : pass-in "alpha" value in SkeletonBody (FSL?)
			float invComeback = 0.95f; // 0 = immediately, 1 = never
			const_cast<b2Vec2&>(body->GetLinearVelocity()) *= invComeback; // doesn't require "friend class"
			body->m_angularVelocity *= invComeback;
		}
	}

	float AnimatedPhysics::boneToBodyRotation(const spBone * const bone, const float & renderRotation)
	{
		return atan2(bone->c, bone->a) + M_PI_2 - renderRotation;
	}

	b2Vec2 AnimatedPhysics::relativeBoneToBodyPosition(const spBone* const bone, const cocos2d::AffineTransform& renderTransform) const
	{
		// no offsets
		b2Vec2 outWorldPosition;
		outWorldPosition.x = bone->worldX * renderTransform.a + bone->worldY * renderTransform.c;
		outWorldPosition.x *= renderToBodyScale;
		outWorldPosition.y = bone->worldX * renderTransform.b + bone->worldY * renderTransform.d;
		outWorldPosition.y *= renderToBodyScale;
		return outWorldPosition;
	}

	b2Vec2 AnimatedPhysics::boneToBodyPosition(const spBone* const bone, const cocos2d::AffineTransform& renderTransform) const
	{
		// new, super-fast way
		b2Vec2 outWorldPosition;
		outWorldPosition.x = bone->worldX * renderTransform.a + bone->worldY * renderTransform.c + renderTransform.tx;
		outWorldPosition.x *= renderToBodyScale;
		outWorldPosition.y = bone->worldX * renderTransform.b + bone->worldY * renderTransform.d + renderTransform.ty;
		outWorldPosition.y *= renderToBodyScale;
		return outWorldPosition;
		/// old, super-slow way
		//outWorldPosition.set(bone->worldX, bone->worldY, 0);
		//renderTransform.transformPoint(&outWorldPosition); // slow
		//outWorldPosition *= renderToBodyScale;
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

	// inverse function of "teleportBodiesToCurrentPose"
	void AnimatedPhysics::matchPoseToBodies()
	{
		// essential to traverse bones hierarchically, update parent transform before childrens'
		deque<b2Body*>::iterator itBodies = b2Bodies.begin();
		if (itBodies == b2Bodies.end()) return;

		auto renderInverseTransform = renderInstance->getWorldToNodeTransform();
		auto renderTransform = renderInstance->getNodeToWorldAffineTransform();
		float renderRotation = atan2(renderTransform.c, renderTransform.a);

		//if (!hasJoints)
		{
			// root bone -> put root body's position to 'renderInstance'
			b2Body* body = *(itBodies);
			cocos2d::Vec3 pos(body->GetPosition().x, body->GetPosition().y, 0);
			pos.x /= renderToBodyScale;
			pos.y /= renderToBodyScale;
			renderInstance->getParent()->getWorldToNodeTransform().transformPoint(&pos);
			renderInstance->setPosition(pos.x, pos.y);
			//CCLOG("RI %s o=%f,%f", static_cast<BodyData*>(body->GetUserData())->bone->data->name, renderInstance->getPosition().x, renderInstance->getPosition().y);
		}

		for (++itBodies; itBodies != b2Bodies.end(); ++itBodies)
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

	// split function for joint/disjoint
	void AnimatedPhysics::impulseBodiesToCurrentPose()
	{
		auto renderTransform = renderInstance->getNodeToWorldAffineTransform();
		float renderRotation = atan2(renderTransform.c, renderTransform.a);

		for (b2Body* body : b2Bodies)
		{
			body->SetAwake(true);
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			spBone* bone = data->bone;

			/// <TODO> : make this user-defined
			// how much the skeleton reacts to external forces
			float damping = .1f; // inverted damping, that is

			if (true && true || !hasJoints)
			{
				// hack-in linear impulse
				b2Vec2 screenPosition = boneToBodyPosition(bone, renderTransform);
				b2Vec2 newImpulse{ screenPosition - body->GetPosition() };
				newImpulse *= invTimeStep;
				b2Vec2& speedRef = const_cast<b2Vec2&>(body->GetLinearVelocity()); // doesn't require "friend class"
				speedRef -= data->previousImpulse;
				speedRef *= damping;
				speedRef += (data->previousImpulse = newImpulse);
			}

			// hack-in angular impulse
			float screenRotation = boneToBodyRotation(bone, renderRotation);
			float newTorque = boundAngleToPI(screenRotation - body->GetAngle());
			newTorque *= invTimeStep;
			body->m_angularVelocity -= data->previousTorque; // not too bad to actually call "SetAngularVelocity", but better make body return "const float&"
			body->m_angularVelocity *= damping;
			body->m_angularVelocity += (data->previousTorque = newTorque);
		}
	}

	void AnimatedPhysics::impulseBodiesRelativelyToPoseChange()
	{
		auto renderTransform = renderInstance->getNodeToWorldAffineTransform();
		float renderRotation = atan2(renderTransform.c, renderTransform.a);

		for (b2Body* body : b2Bodies)
		{
			body->SetAwake(true);
			BodyData* data = static_cast<BodyData*>(body->GetUserData());
			spBone* bone = data->bone;

			//if (true)
			{
				b2Vec2 newRelativePosition = relativeBoneToBodyPosition(bone, renderTransform);
				b2Vec2 newImpulse{ newRelativePosition - data->expectedPos };
				data->expectedPos = newRelativePosition;
				// if we fix position difference, we are in effect fixing gravity and friction! Which is NOT what we want.
				//float maxFixPercent = 0;
				//b2Vec2 difference{ data->expectedPos - body->GetPosition() + renderToBodyScale * b2Vec2(renderTransform.tx, renderTransform.ty) };
				//newImpulse += maxFixPercent * difference;

				newImpulse *= invTimeStep;
				b2Vec2& speedRef = const_cast<b2Vec2&>(body->GetLinearVelocity()); // doesn't require "friend class"
				speedRef -= data->previousImpulse;
				// same here, don't minimize external forces' effect
				//float invDamping = 1.f; //speedRef *= invDamping;
				speedRef += (data->previousImpulse = newImpulse);
				//if (string(bone->data->name) == "right hand") CCLOG("%s %f,%f", bone->data->name, newImpulse.x, newImpulse.y);
			}

			// hack-in angular impulse

			/// <TODO> : make these user-defined
			float damping = .10f; // how much external forces accumulate to animation
			float fixPercent = 1.0f; // how much to fix animation difference every frame from external forces
			float screenRotation = boneToBodyRotation(bone, renderRotation);
			float newTorque = boundAngleToPI(screenRotation - data->expectedAngle);
			float difference = boundAngleToPI(data->expectedAngle - body->GetAngle());
			newTorque += difference * fixPercent;
			data->expectedAngle = screenRotation;

			newTorque *= invTimeStep;
			body->m_angularVelocity -= data->previousTorque; // not too bad to actually call "SetAngularVelocity", but better make body return "const float&"
			body->m_angularVelocity *= damping;
			body->m_angularVelocity += (data->previousTorque = newTorque);
		}
	}

	void AnimatedPhysics::teleportTo(const float & x, const float & y, const float & degrees)
	{
		renderInstance->setPosition(x, y);
		renderInstance->setRotation(degrees);
		teleportBodiesToCurrentPose();
	}

	void AnimatedPhysics::setAnimation(spAnimation * newAnim, const float & timeToAdapt)
	{
		currentAnimation = newAnim;
		newAnimAdaptTime = timeToAdapt;
		animationTime = 0;
	}

	void AnimatedPhysics::setPhysicsAnimationType(const PhysicsAnimationType &type)
	{
		if (currentPhysicsAnimationType == type) return;
		/// <TODO> : to make runtime changes bearable, I should compare the <old> PhysicsAnimationType with the new one and change current body impulses!
		switch (type) {

		case PhysicsAnimationType::TELEPORT:
			physicsAnimationFunction = &AnimatedPhysics::teleportBodiesToCurrentPose;
			for (b2Body* body : b2Bodies)
			{
				BodyData* data = static_cast<BodyData*>(body->GetUserData());
				data->previousImpulse.SetZero(); // set to zero simply in case of changing to impulse later
				data->previousTorque = 0;
			}
			break;

		case PhysicsAnimationType::IMPULSE_ABSOLUTE:
			physicsAnimationFunction = &AnimatedPhysics::impulseBodiesToCurrentPose;
			break;

		case PhysicsAnimationType::IMPULSE_RELATIVE:
			physicsAnimationFunction = &AnimatedPhysics::impulseBodiesRelativelyToPoseChange;
			cocos2d::AffineTransform& renderTransform = renderInstance->getNodeToWorldAffineTransform();
			for (b2Body* body : b2Bodies)
			{
				BodyData* data = static_cast<BodyData*>(body->GetUserData());
				data->expectedPos = relativeBoneToBodyPosition(data->bone, renderTransform);
				data->expectedAngle = body->GetAngle();
			}
			break;
		}

		currentPhysicsAnimationType = type;
	}

	void AnimatedPhysics::runAnimation()
	{
		if (!currentAnimation) return;

		if (!hasJoints) setBonesToSetupPose(); // helps keep bones together

		// events
		int eventsCount;
		spEvent** events;
		if (eventListener) {
			events = spineEvents.data();
			eventsCount = 0;
		}
		else {
			events = nullptr;
		}

		// animate
		{
			float blendValue;
			if (newAnimAdaptTime > 0)
			{
				blendValue = animationTime / newAnimAdaptTime;
				if (blendValue >= 1) {
					newAnimAdaptTime = 0.f;
					blendValue = 1.f;
				}
			}
			else {
				blendValue = 1.f;
			}

			// Spine it!
			spAnimation_apply(currentAnimation, renderInstance->getSkeleton(), animationTime, animationTime += timeStep, true, events, &eventsCount, blendValue, blendValue == 1.f, false);
			renderInstance->updateWorldTransform();

			// Box2D it!
			(this->*physicsAnimationFunction)();
		}
	}
}
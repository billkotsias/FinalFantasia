#pragma once

#include "set"
//#include "map"
#include "Box2D\Box2D.h"

namespace spine {
	class SkeletonRenderer;
}

using namespace std;

namespace psi {

	class SkeletonBody;

	class AnimatedPhysics {

	public:

		AnimatedPhysics(spine::SkeletonRenderer* renderInstance, b2World* physicsWorld, const float& renderToBodyScale, const bool& hasJoints,
			const float& timeStep = 1.f / 60.f);
		virtual ~AnimatedPhysics();

		void insertBody(b2Body* const &, spBone* const &);
		void destroyBody(b2Body* const &);
		void destroyJoints(); // if you want to completely disjoint a skeleton

		// only issue on initialization
		void teleportBodiesToCurrentPose();

		// now we're talking
		inline float boneToBodyRotation(const spBone* const bone, const float& renderRotation);
		inline b2Vec2 boneToBodyPosition(const spBone* const bone, const cocos2d::AffineTransform& renderTransform) const;
		inline b2Vec2 relativeBoneToBodyPosition(const spBone* const bone, const cocos2d::AffineTransform& renderTransform) const;

		void setTimeStep(const float& timeStep) {
			this->timeStep = timeStep;
			invTimeStep = 1.f / timeStep;
		}

		void setBonesToSetupPose();
		/// - set animation frame
		void impulseBodiesToCurrentPose();
		void impulseBodiesRelativelyToPoseChange();
		/// - run physics
		void matchPoseToBodies();
		/// - render

		// convenience functions
		// should be called to position initially
		void teleportTo(const float& x, const float& y, const float& degrees = 0);

		void setAnimation(spAnimation*, const float& timeToAdapt = 0);
		void setAnimation(const char* const animationName, const float& timeToAdapt) { setAnimation(spSkeletonData_findAnimation(renderInstance->getSkeleton()->data, animationName), timeToAdapt); }
		void setAnimation(int animationIndex, const float& timeToAdapt = 0) {
			if (animationIndex < 0)
				setAnimation((spAnimation*)nullptr, timeToAdapt);
			else
				setAnimation(renderInstance->getSkeleton()->data->animations[animationIndex], timeToAdapt);
		}

		enum class PhysicsAnimationType {
			TELEPORT,
			IMPULSE_ABSOLUTE,
			IMPULSE_RELATIVE
		};
		void setPhysicsAnimationType(const PhysicsAnimationType&);
		void runAnimation();

		// getters
		spine::SkeletonRenderer* getRenderable() const { return renderInstance; }
		const float& renderToBodyScale; // render coords * this = body coords. (by referencing, we save 4 bytes, but also state that this is same as SkeletonBody)

		/// <TODO> : events
		// only one listener allowed for performance
		void setEventListener(function<void(AnimatedPhysics*)>eventListener = 0, unsigned int maxEventsExpected = 5) {
			this->eventListener = eventListener;
			spineEvents.reserve(maxEventsExpected);
		}

	private:

		// main
		const bool hasJoints;
		b2World* physicsWorld;
		spine::SkeletonRenderer* renderInstance;
		deque<b2Body*> b2Bodies;
		float timeStep;
		float invTimeStep;

		class BodyData {
		public:
			spBone* bone;
			b2Vec2 previousImpulse{ 0,0 };
			float previousTorque{ 0 };
			b2Vec2 expectedPos;
			float expectedAngle;
		};

		// convenience
		spAnimation* currentAnimation{ nullptr };
		float animationTime{ 0.f };
		float newAnimAdaptTime{ 0.f };
		PhysicsAnimationType currentPhysicsAnimationType;
		void(AnimatedPhysics::* physicsAnimationFunction)() { &AnimatedPhysics::impulseBodiesToCurrentPose };

		/// events
		function<void(AnimatedPhysics*)> eventListener;
		vector<spEvent*> spineEvents; // vector simply used for reserving raw memory, not used directly
	};
}
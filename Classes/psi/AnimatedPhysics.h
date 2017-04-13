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

	class BodyData {
	public:
		spBone* bone;
		b2Vec2 previousImpulse;
		float previousTorque;
	};

	class AnimatedPhysics {

	public:

		AnimatedPhysics(spine::SkeletonRenderer* renderInstance, b2World* physicsWorld, const float& renderToBodyScale);
		virtual ~AnimatedPhysics();

		void insertBody(b2Body* const &, spBone* const &);
		void destroyBody(b2Body* const &);
		void insertJoint(b2Joint* const &);
		void destroyJoints();

		// only issue on initialization
		void teleportBodiesToCurrentPose();

		// now we're talking
		void getBoneScreenTransform(const spBone* const bone, const cocos2d::AffineTransform& renderTransform, const float& renderRotation, cocos2d::Vec3& outWorldPosition, float& outWorldRotation) const;

		/// set animation frame;
		void impulseBodiesToCurrentPose(float timeStep);
		/// - run physics
		void matchPoseToBodies();
		/// - render

		spine::SkeletonRenderer* getRenderable() const { return renderInstance; }
		const float& renderToBodyScale; // render coords * this = body coords. (by referencing, we save 4 bytes, but also state that this is same as SkeletonBody)

	private:

		b2World* physicsWorld;
		spine::SkeletonRenderer* renderInstance;
		set<b2Body*> b2Bodies;
		//map<spBone*, b2Body*>
		set<b2Joint*> b2Joints;

	};
}
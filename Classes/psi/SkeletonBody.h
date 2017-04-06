#pragma once

#include "vector"
#include "Box2D\Box2D.h"

struct spSkeletonData;
struct spAttachment;
struct spSkin;
struct spBone;

struct b2BodyDef;

using namespace std;

namespace spine {
	class SkeletonRenderer;
}

namespace psi {

	class AnimatedPhysics;

	class SkeletonBodyPose {
	public:
		spAnimation* animation;
		float time{ 0.f };
		bool flipX{ false };
		bool flipY{ false };
	};

	class SkeletonBody {

	public:

		typedef pair<b2BodyDef, vector< b2FixtureDef > > BodyDefinition;
		typedef vector<BodyDefinition> BodyDefinitions;
		typedef vector< unique_ptr<b2Shape> > ShapesRegistry;

		static inline b2Vec2 GetWorldScale(spBone*);

		// copy array of floats (x,y pairs) to b2Vec2 array
		static inline void CopyVerticesTob2Vec2Array(std::vector<b2Vec2>& destination, const float* const vertices, const unsigned int floatsCount, const b2Vec2& scale) {
			destination.reserve(floatsCount);
			/// <NOTE> : minus y
			for (unsigned int j = 0; j < floatsCount; ) destination.emplace_back(b2Vec2(vertices[j++] * scale.x, -vertices[j++] * scale.y));
		};
		// a more generic and suitable function than 'spSkeleton_getAttachmentForSlotIndex'
		static spAttachment* GetAttachmentOfSlotIndex(const spSkeletonData* const data, const int slotIndex, const char* const attachmentName, const spSkin* const skin = nullptr);
		static void GetAttachmentVertices(vector< vector<b2Vec2> >& destination, const spAttachment* const _attachment, spSlot* const, const b2Vec2& scale);

		static void CreateScaledFixture(b2Body* body, b2FixtureDef& fixtureDef, float scale);

		// end of static

		// Create automatically an assembly of Box2D body definitions from spSkeletonData
		SkeletonBody(spSkeletonData* const data, const b2Vec2& scale = b2Vec2(1.f, 1.f), string skinName = "", const SkeletonBodyPose* const = nullptr);
		virtual ~SkeletonBody();

		AnimatedPhysics* createInstance(b2World* m_world, float scale);

	private:

		// manage destructors automatically
		ShapesRegistry defaultFixtureShapes;

		spSkeletonData* const skeletonData;
		spine::SkeletonRenderer* defaultRenderInstance;

		BodyDefinitions bodyDefinitions;

		const b2Vec2 renderToBodyScale; // render coords * this = body coords
	};
}

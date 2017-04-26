#pragma once

#include "vector"
#include "deque"
#include "utility"
#include "Box2D\Box2D.h"
#include "polypartition.h"

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

		typedef pair<b2BodyDef, deque< b2FixtureDef > > BodyDefinition;
		typedef vector<BodyDefinition> BodyDefinitions;
		typedef deque< unique_ptr<b2Shape> > ShapesRegistry;

		static inline b2Vec2 GetWorldScale(spBone*);

		// copy array of floats (x,y pairs) to b2Vec2 array
		static inline void CopyVerticesTob2Vec2Array(std::vector<b2Vec2>& destination, const float* const vertices, const unsigned int floatsCount, const b2Vec2& scale) {
			destination.reserve(floatsCount>>1);
			/// <NOTE> : minus <X>?
			for (unsigned int j = 0; j < floatsCount; j+=2) {
				// (y,-x) seems like a crazy bug. Maybe check it out someday.
				destination.emplace_back(vertices[j+1] * scale.x, -vertices[j] * scale.y);
				//destination.push_back(b2Vec2(vertices[j + 1] * scale.x, -vertices[j] * scale.y)); // the same, but potentially slower
			}
		};
		static inline void CopyVerticesTob2Vec2Array(TPPLPoly& hull, const float* const vertices, const unsigned int floatsCount, const b2Vec2& scale) {
			hull.Init(floatsCount >> 1);
			for (unsigned int j = 0; j < floatsCount; j+=2) {
				//CCLOG("WTF2 %f,%f", vertices[j] * scale.x, vertices[j + 1] * scale.y);
				TPPLPoint& point = hull.GetPoint(j >> 1);
				point.x = vertices[j+1] * scale.x;
				point.y = -vertices[j] * scale.y;
			}
		};
		// a more generic and suitable function than 'spSkeleton_getAttachmentForSlotIndex'
		static spAttachment* GetAttachmentOfSlotIndex(const spSkeletonData* const data, const int slotIndex, const char* const attachmentName, const spSkin* const skin = nullptr);
		static void GetAttachmentVertices(vector< vector<b2Vec2> >& destination, const spAttachment* const _attachment, spSlot* const, const float& scale);

		static void CreateScaledFixture(b2Body* body, b2FixtureDef& fixtureDef, float scale);

		// end of static

		// Create automatically an assembly of Box2D body definitions from spSkeletonData
		SkeletonBody(spSkeletonData* const data, const float& scale = 1.f, string skinName = "", const SkeletonBodyPose* const = nullptr);
		virtual ~SkeletonBody();

		AnimatedPhysics* createInstance(b2World* const m_world, const float& scale, const bool& createJoints = true);

	private:

		// manage destructors automatically
		ShapesRegistry defaultFixtureShapes;

		spSkeletonData* const skeletonData;
		spine::SkeletonRenderer* defaultRenderInstance;

		BodyDefinitions bodyDefinitions;

		const float renderToBodyScale; // render coords * this = body coords
	};
}

#include "stdafx.h"
#include "SkeletonBody.h"
#include "AnimatedPhysics.h"
#include "list"

#include "spineCocos\spine\spine-cocos2dx.h"

using namespace std;

namespace psi {

	b2Vec2 SkeletonBody::GetWorldScale(spBone * bone)
	{
		// http://math.stackexchange.com/questions/78137/decomposition-of-a-nonsquare-affine-matrix
		// http://mathforum.org/mathimages/index.php/Transformation_Matrix
		/// <NOTE> : for some reason, b <-> c are inverted by spBone transformations
		float sx = sqrt(bone->a * bone->a + bone->c * bone->c);
		float sy = (bone->a * bone->d - bone->b * bone->c) / sx; // faster than spBone's (no sqrt) but unsafer!
		//CCLOG("COMPARE %i %f %f", sy != spBone_getWorldScaleY(bone), sy, spBone_getWorldScaleY(bone));
		return b2Vec2(sx, sy);
		//return b2Vec2(spBone_getWorldScaleX(bone), spBone_getWorldScaleY(bone));
	}

	spAttachment * SkeletonBody::GetAttachmentOfSlotIndex(
		const spSkeletonData * const skeletonData, const int slotIndex, const char * const attachmentName, const spSkin * const skin)
	{
		if (skin) {
			spAttachment *attachment = spSkin_getAttachment(skin, slotIndex, attachmentName);
			if (attachment) return attachment;
		}
		if (skeletonData->defaultSkin) {
			spAttachment *attachment = spSkin_getAttachment(skeletonData->defaultSkin, slotIndex, attachmentName);
			return attachment;
		}
		return nullptr;
	}

	void SkeletonBody::GetAttachmentVertices(std::vector<std::vector<b2Vec2>>& arrayOfVertices, const spAttachment* const _attachment, spSlot* const slot, const float& _scale)
	{
		//std::vector<b2Vec2> b2Vecs; // x,y pairs
		TPPLPoly hull;
		spBone* const parentBone = slot->bone;
		b2Vec2 scale = GetWorldScale(parentBone);
		scale *= _scale;
		bool checkConvex = false;

		/// <NOTE> : bone scaling is "burned-in" cause we can only take it into account at b2Body-creation time
		auto parseVertexAttachment = [&](spVertexAttachment& vAttachment, unsigned int verticesCount /*hull-only*/)
		{
			checkConvex = true;
			if (!vAttachment.bonesCount) {
				float* sourceVerticesPtr = vAttachment.vertices;
				CopyVerticesTob2Vec2Array(hull, sourceVerticesPtr, verticesCount, scale); // no bone influences, copy vertices directly
			}
			else {
				CCLOG("DEFORMING-APPROXIMATION ?!!!!!");
				/// <NOTE> : experimental stuff, Box2D bodies aren't allowed to deform
				/// instead, we just make an approximation based on original world coords and parent bone transformation
				auto output = make_unique<float[]>(verticesCount);
				spVertexAttachment_computeWorldVertices(&vAttachment, slot, 0, verticesCount, output.get(), 0, 2);
				/// <TODO> : check if we must truly remove parent bone's transformation ?!
				for (unsigned int i = 0; i < verticesCount; i += 2) {
					spBone_worldToLocal(parentBone, output[i], output[i + 1], &output[i], &output[i + 1]);
				}
				CopyVerticesTob2Vec2Array(b2Vecs, output.get(), verticesCount, scale);
			}
		};

		auto parseRegionAttachment = [&](const spRegionAttachment& rAttachment)
		{
			const float* sourceVerticesPtr = rAttachment.offset; // local coords, don't take into account parent bones' transforms
			CopyVerticesTob2Vec2Array(hull, sourceVerticesPtr, 8, scale); // no bone influences, copy vertices directly
		};

		switch (_attachment->type)
		{
			case SP_ATTACHMENT_REGION:
				parseRegionAttachment(*(spRegionAttachment*)_attachment);
				CCLOG("REGION vertices:%i", b2Vecs.size());
				break;

			case SP_ATTACHMENT_BOUNDING_BOX:
			{
				spVertexAttachment vertexAttachment = ((spBoundingBoxAttachment*)_attachment)->super;
				parseVertexAttachment(vertexAttachment, vertexAttachment.verticesCount);
				CCLOG("BOUNDS vertices:%i", b2Vecs.size());
				break;
			}

			case SP_ATTACHMENT_MESH:
			{
				spMeshAttachment* meshAttachment = (spMeshAttachment*)_attachment;
				parseVertexAttachment(meshAttachment->super, meshAttachment->hullLength);
				CCLOG("MESH vertices:%i", b2Vecs.size());
				break;
			}

				/// <TODO> : Implement something!
				//case SP_ATTACHMENT_CIRCLE:
				//	break;

			default:
				return; // empty handed
		}
		if (b2Vecs.empty()) return;

		// make sure hull is convex
		if (checkConvex) {

		}
		else {

		}

		if (b2Vecs.size() > 8)
		{
			/// <TODO> : box 2d doesn't like tons of vertices - so use <PolyPartition> library for this
			TPPLPoly poly;
			poly.Init(b2Vecs.size());
			for (int i = 0; i < poly.GetNumPoints(); ++i)
			{
				TPPLPoint& point = poly.GetPoint(i);
				b2Vec2& vec = b2Vecs[i];
				point.x = vec.x; // I am tired of copying the SAME 2D point over and over again
				point.y = vec.y;
			}
			list<TPPLPoly> polyList;
			TPPLPartition::ConvexPartition_HM(&poly, &polyList);
			CCLOG("\nARGHHHHHHH:%d\n", polyList.size());
			for (TPPLPoly poly : polyList) {
				list<TPPLPoly> polyList;
				TPPLPartition::Triangulate_EC(&poly, &polyList);
				CCLOG("\nFUCK THIS:%d,%d\n", b2Vecs.size(), polyList.size());
				for (TPPLPoly triangle : polyList) {
					b2Vecs.clear();
					CCLOG("points = %d", triangle.GetNumPoints());
					for (int i = 0; i < triangle.GetNumPoints(); ++i) {
						b2Vecs.push_back(b2Vec2(triangle[i].x, triangle[i].y));
					}
					arrayOfVertices.push_back(b2Vecs);
				}
			}
			//b2Vecs.resize(8);
			//arrayOfVertices.push_back(b2Vecs);
		}
		else
		{
			//arrayOfVertices.push_back(b2Vecs);
		}
	}

	void SkeletonBody::CreateScaledFixture(b2Body* body, b2FixtureDef& fixtureDef, float scale)
	{
		if (scale == 1.f) {
			body->CreateFixture(&fixtureDef);
			return;
		}

		b2FixtureDef scaledFixture(fixtureDef);
		const b2Shape* const sourceShape = fixtureDef.shape;
		b2Shape* scaledShape;
		b2Shape::Type shapeType = sourceShape->GetType();
		switch (shapeType)
		{
			case b2Shape::e_circle:
			{
				b2CircleShape* shape = (b2CircleShape*)(scaledShape = new b2CircleShape(*(b2CircleShape*)sourceShape));
				shape->m_p *= scale;
				shape->m_radius *= scale;
				break;
			}
			case b2Shape::e_polygon:
				if (false)
				{
					// safe but super-slow cloning
					b2PolygonShape* destShape = (b2PolygonShape*)(scaledShape = new b2PolygonShape());
					b2PolygonShape* sourcePolyShape = (b2PolygonShape*)sourceShape;
					const int32& vertexCount = sourcePolyShape->GetVertexCount();
					auto scaledVecs = make_unique<b2Vec2[]>(vertexCount);
					for (int32 i = 0; i < vertexCount; ++i) {
						const b2Vec2& sourceVertex = sourcePolyShape->GetVertex(i);
						scaledVecs[i].Set(sourceVertex.x * scale, sourceVertex.y * scale);
					}
					destShape->Set(scaledVecs.get(), vertexCount);
					break;
				}
				else {
					/// <TODO> : a lot-lot-lot faster but must be tested extensively
					b2PolygonShape* sourcePolyShape = (b2PolygonShape*)sourceShape;
					b2PolygonShape* destShape = (b2PolygonShape*)(scaledShape = new b2PolygonShape(*sourcePolyShape));
					const int32& vertexCount = sourcePolyShape->GetVertexCount();
					for (int32 i = 0; i < vertexCount; ++i) {
						b2Vec2& dest = destShape->m_vertices[i];
						b2Vec2& src = sourcePolyShape->m_vertices[i];
						dest.x = src.x * scale;
						dest.y = src.y * scale;
					}
					destShape->m_centroid = b2PolygonShape::ComputeCentroid(destShape->m_vertices, vertexCount);
					break;
				}
		}

		scaledFixture.shape = scaledShape;
		body->CreateFixture(&scaledFixture);
		delete scaledShape;
	}

	//

	SkeletonBody::SkeletonBody(spSkeletonData* const skeletonData, const float& scale, string skinName, const SkeletonBodyPose* const initPose) :
		skeletonData(skeletonData), renderToBodyScale(scale)
	{
		// create and setup skeleton before parsing
		defaultRenderInstance = spine::SkeletonRenderer::createWithData(skeletonData, false);
		//defaultRenderInstance->retain(); /// !
		defaultRenderInstance->setSkin(skinName);
		spSkeleton* skeleton = defaultRenderInstance->getSkeleton();
		if (initPose) {
			spAnimation_apply(initPose->animation, skeleton, initPose->time, initPose->time, false, nullptr, nullptr, 1.f, true, false);
		}
		else {
			spSkeleton_setToSetupPose(skeleton);
		}
		spSkeleton_updateWorldTransform(skeleton);

		// create b2BodyDef for each bone (keep bone hierarchy)
		vector<BodyDefinition*> boneIndexToBodyDefinition;
		boneIndexToBodyDefinition.resize(skeleton->bonesCount);
		bodyDefinitions.resize(skeleton->bonesCount);

		unsigned int hierarchicalBoneIndex = 0;
		list<spBone*> bonesToParse;
		bonesToParse.push_back(skeleton->root);
		while (!bonesToParse.empty())
		{
			// populate list of bones to parse
			spBone* const bone = bonesToParse.front();
			bonesToParse.pop_front();
			for (int i = bone->childrenCount - 1; i >= 0; --i)
			{
				bonesToParse.push_back(bone->children[i]);
			}

			BodyDefinition* bodyDefinition = &bodyDefinitions.at(hierarchicalBoneIndex);

			int boneIndex = bone->data->index;
			boneIndexToBodyDefinition[boneIndex] = bodyDefinition;

			b2BodyDef* bodyDef = &bodyDefinition->first;
			bodyDef->userData = (void*)boneIndex; // register bone's index in skeleton
			bodyDef->type = b2_dynamicBody; /// <TODO> : kinetic may be desirable in some cases
			bodyDef->gravityScale = 1; /// <TODO> : gravityScale must be zero when absolute-impulsing disjointed models!
			bodyDef->fixedRotation = false;
			// this must be zero when impulsing, and with teleporting an easier-to-grasp value is user-defined, elsewhere (0 to 1)
			bodyDef->angularDamping = 0.f;
			bodyDef->linearDamping = 0.f;
			bool isBullet = false; /// <TODO> : use naming convention for <isBullet>
			bodyDef->bullet = isBullet;
			//b2BodyDef->position = b2Vec2(position.x, position.y);
			++hierarchicalBoneIndex;
		}

		// create b2FixtureDef for each slot/attachment
		const unsigned int slotsCount = skeleton->slotsCount;
		spSlot** const slots = skeleton->slots;
		for (unsigned int i = 0; i < slotsCount; ++i)
		{
			spSlot* const slot = slots[i];
			/// <NOTE 1> : we are only interested in slots that have a default attachment in the <current pose>
			/// <NOTE 2> : we are only interested in specific attachments
			spAttachment* _attachment = slot->attachment; // old : GetAttachmentOfSlotIndex(skeletonData, i, slot->attachmentName, skin);
			if (!_attachment) continue;
			vector< vector<b2Vec2> > b2Vecs;
			GetAttachmentVertices(b2Vecs, _attachment, slot, renderToBodyScale);
			if (b2Vecs.empty()) continue;

			unsigned int fixtureCount = b2Vecs.size();
			CCLOG("Fixtures to define:%i", fixtureCount);

			BodyDefinition* bodyDefinition = boneIndexToBodyDefinition[slot->bone->data->index];
			deque<b2FixtureDef>* bodyDefFixtures = &bodyDefinition->second;

			while (fixtureCount--)
			{
				vector<b2Vec2>& vertices = b2Vecs[fixtureCount];
				unique_ptr<b2Shape> fixtureShape;
				if (vertices.size() == 2) {
					fixtureShape = unique_ptr<b2Shape>(new b2CircleShape());
					b2CircleShape* circleShape = static_cast<b2CircleShape*>(fixtureShape.get());
					circleShape->m_p = vertices[0];
					circleShape->m_radius = (vertices[1] - vertices[0]).Length(); /// <TODO> : Set a standard for defining circle shapes
					//circleShape->m_radius = vertices[1].x; // perhaps
				}
				else {
					fixtureShape = unique_ptr<b2Shape>(new b2PolygonShape());
					b2PolygonShape* polygonShape = static_cast<b2PolygonShape*>(fixtureShape.get());
					polygonShape->Set(vertices._Get_data()._Myfirst /* copied over */, vertices.size());
				}
				/// create <temporary> fixture(s) from vertices
				/// prior to creating final body fixtures, fixture defs' shapes will be <re-scaled to Skeleton Instance's actual scale (if != 1)!!!>
				/// note that we'll have to use the final <bone> scale!

				bodyDefFixtures->emplace_back(b2FixtureDef());
				b2FixtureDef* fixtureDef = &bodyDefFixtures->back();
				fixtureDef->shape = fixtureShape.get();
				fixtureDef->filter.groupIndex = -1; /// <TODO> : same for all enemies, changes when one becomes physics-controlled
				/// <TODO> : below should probably be user defined when creating SkeletonBody
				fixtureDef->density = 2.f;
				fixtureDef->friction = 0.5f;
				fixtureDef->restitution = 0.f;
				//fixtureDef->filter.categoryBits = PHYSICS_FILTER_CATEGORY_ENEMY;
				//fixtureDef->filter.maskBits = PHYSICS_FILTER_MASK_ENEMY;
				defaultFixtureShapes.push_back(std::move(fixtureShape)); // register shape for deletion when SkeletonBody dies
			}
		}
		//CCLOG("<=====");
		//CCLOG("BODY DEFS DEFINED");
		//for (BodyDefinition bodyDef : bodyDefinitions) {
		//	CCLOG("Defined body %i with %i fixtures", bodyDef.first.userData, bodyDef.second.size());
		//}
		//CCLOG("=====>");
	}

	SkeletonBody::~SkeletonBody()
	{
	}

	AnimatedPhysics* SkeletonBody::createInstance(b2World * m_world, const float& scale, const bool& createJoints)
	{
		// do NOT create "bone-to-body" map if we are not creating joints. If created, we want it to get auto-destroyed at end of this function
		// now THAT's optimization
		typedef map<spBone*, b2Body*> BoneToBodyMap;
		unique_ptr< BoneToBodyMap > boneToBodyMapPtr( createJoints ? new map<spBone*, b2Body*> : nullptr );
		BoneToBodyMap& boneToBodyMap = *boneToBodyMapPtr.get();
		const float finalScale = renderToBodyScale * scale;
		
		spine::SkeletonRenderer* renderInstance;
		if (defaultRenderInstance) {
			renderInstance = defaultRenderInstance;
			defaultRenderInstance = nullptr;
		}
		else {
			renderInstance = spine::SkeletonRenderer::createWithData(skeletonData, false); // own=true => destroy the data when instance expires!
		}
		/// <TODO> : further change scale below to match GLES-RENDERER scale, in case a character has different scale to <common> renderToBodyScale<!>
		renderInstance->setScale(scale);
		auto skeletonInstance = renderInstance->getSkeleton();
		AnimatedPhysics* animated = new AnimatedPhysics(renderInstance, m_world, renderToBodyScale, createJoints);

		for (BodyDefinition& bodyDefinition : bodyDefinitions)
		{
			b2Body* body = m_world->CreateBody(&bodyDefinition.first);
			spBone* bone = skeletonInstance->bones[(int)body->GetUserData()];
			animated->insertBody(body, bone);

			// or maybe we DON'T want the skeleton root to rotate due to physics. Only move.
			if (false && bone->data->name == string("root")) {
				b2CircleShape* shape = new b2CircleShape();
				defaultFixtureShapes.emplace_back(unique_ptr<b2Shape>(shape));
				shape->m_radius = renderToBodyScale;
				b2Filter noCollisions;
				noCollisions.maskBits = 0;
				body->CreateFixture(shape, 1)->SetFilterData(noCollisions);
			}

			for (b2FixtureDef& fixtureDef : bodyDefinition.second) {
				CreateScaledFixture(body, fixtureDef, scale);
			}
			
			if (createJoints) boneToBodyMap[bone] = body; // create joints?
		}

		if (createJoints)
		{
			// maybe below 3 lines are useless, but let's try them first
			renderInstance->setToSetupPose();
			//renderInstance->updateWorldTransform(); // needed?
			animated->teleportBodiesToCurrentPose();
			for (BoneToBodyMap::iterator it = boneToBodyMap.begin(); it != boneToBodyMap.end(); ++it)
			{
				spBone* bone = it->first;
				//if (bone->data->name == string("right foot")) continue; // debug: remove
				spBone* parentBone = bone->parent;
				if (parentBone)
				{
					b2Body* body = it->second;
					b2Body* parentBody = boneToBodyMap[parentBone];

					b2RevoluteJointDef jointDef;
					jointDef.bodyA = parentBody;
					jointDef.localAnchorA = parentBody->GetLocalPoint(body->GetPosition());
					jointDef.bodyB = body;
					jointDef.localAnchorB = b2Vec2_zero;
					jointDef.collideConnected = false;
					/// <TODO> : implement CONSTRAINTS <!>
					jointDef.upperAngle = MATH_DEG_TO_RAD(0);
					jointDef.lowerAngle = -jointDef.upperAngle;
					jointDef.enableLimit = false;
					b2RevoluteJoint* joint = static_cast<b2RevoluteJoint*>(m_world->CreateJoint(&jointDef));
					//animated->insertJoint(joint);
				}
			}
		}
		return animated;
	}
}
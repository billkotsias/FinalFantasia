﻿#include "stdafx.h"

#include "HelloWorldScene.h"
#include "Paths.h"
#include "AppDesign.h"

#include "GLES-Render.h"
#include "psi/SkeletonBody.h"
#include "psi/AnimatedPhysics.h"

//#include "SimpleAudioEngine.h"

USING_NS_CC;
using namespace PsiEngine;

Scene* HelloWorld::createScene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    auto layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Layer::init() )
    {
        return false;
    }

	EventListenerMouse* _mouseListener = EventListenerMouse::create();
	_mouseListener->onMouseMove = CC_CALLBACK_1(HelloWorld::onMouseMove, this);
	_mouseListener->onMouseUp = CC_CALLBACK_1(HelloWorld::onMouseUp, this);
	_mouseListener->onMouseDown = CC_CALLBACK_1(HelloWorld::onMouseDown, this);
	_mouseListener->onMouseScroll = CC_CALLBACK_1(HelloWorld::onMouseScroll, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(_mouseListener, this);

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.

    // add a "close" icon to exit the progress. it's an autorelease object
    auto closeItem = MenuItemImage::create(Paths::RES + "CloseNormal.png",
										   Paths::RES + "CloseSelected.png",
                                           CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));
    
    closeItem->setPosition(Vec2(origin.x + visibleSize.width - closeItem->getContentSize().width/2 ,
                                origin.y + closeItem->getContentSize().height/2));

    // create menu, it's an autorelease object
    auto menu = Menu::create(closeItem, NULL);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 1);

    /////////////////////////////
    // 3. add your codes below...

    // add a label shows "Hello World"
    // create and initialize a label
    
    auto label = Label::createWithTTF(Paths::RES + "Hello World", Paths::RES + "fonts/Marker Felt.ttf", 24);
    
    // position the label on the center of the screen
    label->setPosition(Vec2(origin.x + visibleSize.width/2,
                            origin.y + visibleSize.height - label->getContentSize().height));

    // add the label as a child to this layer
    this->addChild(label, 1);

    // add "HelloWorld" splash screen"
    auto sprite = Sprite::create(Paths::RES + "HelloWorld.png");
	sprite->setAnchorPoint(Vec2());
	sprite->setPosition(Vec2());
	sprite->setScale(0.25f);

    // add the sprite as a child to this layer
    this->addChild(sprite, 0);
    
	// spine test
	const string PATH = Paths::RES + "spineTest/";

	string testfile = "goblins";
	spAtlas* atlas = spAtlas_createFromFile((PATH + testfile + ".atlas").c_str(), 0);			// parses atlas
	spAttachmentLoader* attachmentLoader = &(Cocos2dAttachmentLoader_create(atlas)->super);	// knows how to atlas -> cocos2dx textures
	spSkeletonJson* json = spSkeletonJson_createWithLoader(attachmentLoader);				// create json loader with help from above
	json->scale = 1.f;
	spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, (PATH + testfile + ".json").c_str());	// parsed json -> skeleton data
	spSkeletonJson_dispose(json); // no longer needed, since we have our data
	spAtlas_dispose(atlas);							/// <TODO> : when to destroy this<???>
	spAttachmentLoader_dispose(attachmentLoader);	/// <TODO> : when to destroy this<???>
	if (!skeletonData) {
		CCLOG("OOPSA: %s\n", json->error);
		//exit(0);
	}
	string skinName = "goblin";
	/// <NOTE> : this depends on source Spine data, the bigger source, the lower we should scale to bring inside 0.1->10 Box2D size units
	/// ideally, all characters in a game will have the same source scale and thus share the same number here
	float renderToBodyScale = 0.01f;
	float posY = 10;
	for (int i = 0; i < 1; ++i)
	{
		bd = new psi::SkeletonBody(skeletonData, renderToBodyScale, skinName);
		chara = bd->createInstance(&physicsWorld, 1.f, true);
		chara->setAnimation(0);
		chara->setPhysicsAnimationType(psi::AnimatedPhysics::PhysicsAnimationType::IMPULSE_RELATIVE);
		addChild(chara->getRenderable());
		chara->teleportTo(500, posY + 50, 0);
		chara->getRenderable()->setDebugBonesEnabled(true);

		skeletonRenderInstance = spine::SkeletonRenderer::createWithData(skeletonData, false); // own=true => destroy the data when instance expires!
		addChild(skeletonRenderInstance);
		skeletonRenderInstance->setVisible(true);
		//skeletonInstance->setPosition(random(0., 1000.), random(0., 700.));
		skeletonRenderInstance->setPosition(200,200);
		//skeletonInstance->addAnimation(0, "walk", true)->delay = random(0.,1.);
		skeletonRenderInstance->setSkin(skinName);
		skeletonRenderInstance->setOpacityModifyRGB(true);
		skeletonRenderInstance->setDebugBonesEnabled(true);
		skeletonRenderInstance->setDebugSlotsEnabled(true);

		//skeletonRenderInstance->setToSetupPose();
		spSkeleton* skeleton = skeletonRenderInstance->getSkeleton();
		for (int i = 0, n = skeleton->slotsCount; i < n; i++) {
			spSlot* slot = skeleton->slots[i]; // or drawOrder, it's the same
			spBone* bone = slot->bone;
			spAttachment* attachment = slot->attachment;
			if (!attachment) continue;
			if (attachment->type == SP_ATTACHMENT_REGION) {
				spRegionAttachment* regAttachment = (spRegionAttachment*)attachment;
				/// <TODO> : create <b2Body> from 
				/// - <uvs> ( float offset[8]; float uvs[8]; )
				/// - but after applying <local transform> (float x, y, scaleX, scaleY, rotation, width, height)
				/// - but after <combining> it with <bone's> world scaleX, scaleY
				regAttachment->uvs;
			}
		}

		spBone** bones = skeleton->bones;
		for (int j = skeleton->bonesCount - 1; j >= 0; --j)
		{
			spBone* bone = bones[j];
			bone->rotation += 180.f;
		}
		skeleton->root->rotation = 0;
		skeletonRenderInstance->updateWorldTransform();
	}

	scheduleUpdate();

	// enable box2d debug draw
	uint32 flags = 0;
	flags += b2Draw::e_shapeBit;
	flags += b2Draw::e_jointBit;
	flags += b2Draw::e_aabbBit;
	flags += b2Draw::e_pairBit;
	flags += b2Draw::e_centerOfMassBit;
	flags += b2Draw::e_aabbBit;
	auto debugDraw = new GLESDebugDraw(1.f / renderToBodyScale);
	debugDraw->SetFlags(flags);
	physicsWorld.SetDebugDraw(debugDraw);

	b2BodyDef myBodyDef;
	myBodyDef.type = b2_staticBody;
	myBodyDef.position.Set(AppDesign::resolutionSize.width / 2, posY); //set the starting position
	myBodyDef.position *= renderToBodyScale;
	myBodyDef.angle = 0; //set the starting angle
	b2Body* dynamicBody = physicsWorld.CreateBody(&myBodyDef);
	b2PolygonShape boxShape;
	boxShape.SetAsBox(AppDesign::resolutionSize.width * renderToBodyScale, posY * renderToBodyScale);
	b2FixtureDef boxFixtureDef;
	boxFixtureDef.shape = &boxShape;
	boxFixtureDef.restitution = 0.1f;
	boxFixtureDef.friction = 1;
	dynamicBody->CreateFixture(&boxFixtureDef);
	return true;
}

void HelloWorld::update(float deltaTime)
{
	static float curTime = 0.f;
	static float nextBall = 0.1f;

	deltaTime = 1.f / 60;
	Node::update(deltaTime);

	chara->setTimeStep(deltaTime);
	if (playAnim) chara->runAnimation();
	physicsWorld.Step(deltaTime, 10, 4);
	chara->matchPoseToBodies();

	curTime += deltaTime;
	//
	// use Animation 0 (walk)
	///<setupPose> Controls mixing when alpha < 1. When true the value from the timeline is mixed with the value from the setup pose.When false the value from the timeline is mixed with the value from the current pose.Passing true when alpha is 1 is slightly more efficient for most timelines.
	///<mixingOut> True when changing alpha over time toward 0 (the setup or current pose), false when changing alpha toward 1 (the timeline's pose). Used for timelines which perform instant transitions, such as DrawOrderTimeline or AttachmentTimeline.
	spSkeleton* testSkel = skeletonRenderInstance->getSkeleton();
	spAnimation** anims = testSkel->data->animations;
	spSkeleton_setBonesToSetupPose(testSkel);
	spAnimation_apply(anims[0], testSkel, /* only useful for events */ curTime - deltaTime / 1.f, curTime, true, NULL, NULL, 1.f, true, false);
	//spSkeleton_findBone(testSkel, "head")->x += 100.f;
	spSkeleton_updateWorldTransform(testSkel);

	nextBall -= deltaTime;
	static b2Vec2 testBalls = b2Vec2(
		(chara->getRenderable()->getPosition().x + 200) * chara->renderToBodyScale,
		(chara->getRenderable()->getPosition().y + 150) * chara->renderToBodyScale
	);
	if (false && nextBall <= 0 && curTime < 5) {
		nextBall += 0.3f;
		b2BodyDef bdef;
		bdef.type = b2_dynamicBody; //this will be a dynamic body
		bdef.position = testBalls;
		b2Body* body = physicsWorld.CreateBody(&bdef);
		b2FixtureDef fix;
		b2CircleShape sh;
		sh.m_radius = 10 * chara->renderToBodyScale;
		fix.shape = &sh;
		fix.density = 10;
		fix.restitution = .3f;
		body->SetBullet(false);
		body->CreateFixture(&fix);
		body->SetLinearVelocity(chara->renderToBodyScale * b2Vec2(-1000.f, 0.f));
	}
}

void HelloWorld::draw(cocos2d::Renderer *renderer, const cocos2d::Mat4& transform, uint32_t flags)
{
	cocos2d::Layer::draw(renderer, transform, flags);

	GL::enableVertexAttribs(GL::VERTEX_ATTRIB_FLAG_POSITION);
	Director::getInstance()->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
	physicsWorld.DrawDebugData();

	Director::getInstance()->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
}

void HelloWorld::menuCloseCallback(Ref* pSender)
{
    //Close the cocos2d-x game scene and quit the application
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	exit(0);
#endif
    
    /*To navigate back to native iOS screen(if present) without quitting the application, do not use Director::getInstance()->end() and exit(0) as given above,instead trigger a custom event created in RootViewController.mm as below*/
    
    //EventCustom customEndEvent("game_scene_close_event");
    //_eventDispatcher->dispatchEvent(&customEndEvent);
}

void HelloWorld::onMouseDown(Event *event)
{
	// to illustrate the event....
	EventMouse* e = (EventMouse*)event;
	playAnim = !playAnim;
	CCLOG("Mouse Down detected, Key:%d, play:%d", e->getMouseButton(), playAnim);
}

void HelloWorld::onMouseUp(Event *event)
{
	// to illustrate the event....
	EventMouse* e = (EventMouse*)event;
	CCLOG("Mouse Up detected, Key:%d", e->getMouseButton());
}

void HelloWorld::onMouseMove(Event *event)
{
	// to illustrate the event....
	EventMouse* e = (EventMouse*)event;
	//CCLOG("Mouse Position X:%f, Y:%f", e->getCursorX(), e->getCursorY());
	chara->getRenderable()->setPosition(e->getCursorX(), e->getCursorY());
}

void HelloWorld::onMouseScroll(Event *event)
{
	// to illustrate the event....
	EventMouse* e = (EventMouse*)event;
	CCLOG("Mouse Scroll X:%f, Y:%f", e->getScrollX(), e->getScrollY());
}

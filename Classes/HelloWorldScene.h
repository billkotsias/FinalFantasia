#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

namespace psi {
	class SkeletonBody;
	class AnimatedPhysics;
}

class HelloWorld : public cocos2d::Layer
{
public:

    static cocos2d::Scene* createScene();
    virtual bool init();
    
    // a selector callback
    void menuCloseCallback(cocos2d::Ref* pSender);
    
    // implement the "static create()" method manually
    CREATE_FUNC(HelloWorld);

	virtual void update(float deltaTime) override;
	virtual void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4& transform, uint32_t flags) override;

private:
	
	spine::SkeletonRenderer* skeletonRenderInstance;
	b2World physicsWorld{ b2Vec2(0,-10) };
	psi::SkeletonBody* bd;
	psi::AnimatedPhysics* chara;
};

#endif // __HELLOWORLD_SCENE_H__

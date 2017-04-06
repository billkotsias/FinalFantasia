#include "stdafx.h"
#include "AppDesign.h"
#include "AppDelegate.h"
#include "HelloWorldScene.h"

USING_NS_CC;

// below stuff is only used for multiple-sized assets!
static bool useMultisizedAssets = false;
typedef std::pair<int, std::string> Resource;
static Resource smallResolutionRes{ 320, "smallRes" };		// /2.4
static Resource mediumResolutionRes{ 768, "mediumRes" };	//
static Resource largeResolutionRes{ 1536, "highRes" };		// *2

AppDelegate::AppDelegate()
{
}

AppDelegate::~AppDelegate() 
{
}

// if you want a different context, modify the value of glContextAttrs
// it will affect all platforms
void AppDelegate::initGLContextAttrs()
{
    // set OpenGL context attributes: red,green,blue,alpha,depth,stencil
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8};

    GLView::setGLContextAttrs(glContextAttrs);
}

// if you want to use the package manager to install more packages,  
// don't modify or remove this function
static int register_all_packages()
{
    return 0; //flag for packages manager
}

bool AppDelegate::applicationDidFinishLaunching()
{
	Size designResolutionSize{ AppDesign::resolutionSize };

	// initialize director
	auto director = Director::getInstance();
	GLView* glview = director->getOpenGLView();
	if (!glview) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
		glview = GLViewImpl::createWithRect("MyNewGame", cocos2d::Rect(0, 0, designResolutionSize.width, designResolutionSize.height), 1.f, true);
		//glview = GLViewImpl::createWithRect("MyNewGame", cocos2d::Rect(0, 0, 800, 600), 1.f, true);
		//glview = GLViewImpl::createWithFullScreen("MyNewGame");
		//glview = GLViewImpl::create("MyNewGame", true);
#else
		glview = GLViewImpl::create("MyNewGame", true);
#endif
		director->setOpenGLView(glview);
	}

	// turn on display FPS
#ifdef _DEPLOY
	director->setDisplayStats(false);
#else
	director->setDisplayStats(true);
#endif

    // set FPS. the default value is 1.0/60 if you don't call this
    director->setAnimationInterval(1.0f / 60);

	// Set the design resolution
	glview->setDesignResolutionSize(designResolutionSize.width, designResolutionSize.height, ResolutionPolicy::SHOW_ALL);
	auto frameSize = glview->getFrameSize();
	// below stuff is only used for multiple-sized assets!
	// if the frame's height is larger than the height of medium size.
	if (useMultisizedAssets) {
		if (frameSize.height > mediumResolutionRes.first)
		{
			FileUtils::getInstance()->addSearchPath(largeResolutionRes.second);
			director->setContentScaleFactor(largeResolutionRes.first / designResolutionSize.height);
		}
		// if the frame's height is larger than the height of small size.
		else if (frameSize.height > smallResolutionRes.first)
		{
			FileUtils::getInstance()->addSearchPath(mediumResolutionRes.second);
			director->setContentScaleFactor(mediumResolutionRes.first / designResolutionSize.height);
		}
		// if the frame's height is smaller than the height of medium size.
		else
		{
			FileUtils::getInstance()->addSearchPath(smallResolutionRes.second);
			director->setContentScaleFactor(smallResolutionRes.first / designResolutionSize.height);
		}
	}

    register_all_packages();

    // create a scene. it's an autorelease object
    auto scene = HelloWorld::createScene();

    // run
    director->runWithScene(scene);

    return true;
}

// This function will be called when the app is inactive. Note, when receiving a phone call it is invoked.
void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();

    // if you use SimpleAudioEngine, it must be paused
    // SimpleAudioEngine::getInstance()->pauseBackgroundMusic();
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();

    // if you use SimpleAudioEngine, it must resume here
    // SimpleAudioEngine::getInstance()->resumeBackgroundMusic();
}

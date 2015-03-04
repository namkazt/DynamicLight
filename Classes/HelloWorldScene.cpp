#include "HelloWorldScene.h"

USING_NS_CC;

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
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

	shadowCaster = Sprite::create("test.png");
	shadowCaster->retain();
	shadowCaster->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));
	this->addChild(shadowCaster, 10);

	lightSize = 256;
	
	dynLight = DynamicLight::create();
	dynLight->retain();
	//dynLight->debugDrawEnabled = true;
	dynLight->setShadowCasters(shadowCaster);
	dynLight->setColor(Color4B::WHITE);
	dynLight->setPosition({ 300,150 });
	this->addChild(dynLight, 9);
	
    //////////////////////////////////////////////////////////////////////
	// Setup listener
	auto dispatcher = Director::getInstance()->getEventDispatcher();
	auto listener = EventListenerTouchOneByOne::create();
	listener->onTouchBegan = CC_CALLBACK_2(HelloWorld::onTouchBegan, this);
	listener->onTouchMoved = CC_CALLBACK_2(HelloWorld::onTouchMoved, this);
	listener->onTouchEnded = CC_CALLBACK_2(HelloWorld::onTouchEnded, this);
	dispatcher->addEventListenerWithSceneGraphPriority(listener, this);
	auto keylistener = EventListenerKeyboard::create();
	keylistener->onKeyPressed = CC_CALLBACK_2(HelloWorld::onKeyPressed, this);
	keylistener->onKeyReleased = CC_CALLBACK_2(HelloWorld::onKeyReleased, this);
	dispatcher->addEventListenerWithSceneGraphPriority(keylistener, this);
    return true;
}



///////////////////////////////////////////////////////////////////////
/// INPUT EVENT LISNTENER
///////////////////////////////////////////////////////////////////////
void HelloWorld::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
	if (keyCode == EventKeyboard::KeyCode::KEY_1)
	{
		lightSize -= 32;
		dynLight->setLightSize(lightSize);
	}
	if (keyCode == EventKeyboard::KeyCode::KEY_2)
	{
		lightSize += 32;
		dynLight->setLightSize(lightSize);
	}
}

void HelloWorld::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event)
{
	
}

bool HelloWorld::onTouchBegan(Touch* touch, Event* event)
{
	return true;
}

void HelloWorld::onTouchMoved(Touch* touch, Event* event)
{
	Point pos = touch->getLocation();
	log("Tile: %f-%f", pos.x, pos.y);

	dynLight->setPosition(pos);
}


void HelloWorld::onTouchEnded(Touch* touch, Event* event)
{
}
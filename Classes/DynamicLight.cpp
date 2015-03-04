#include "DynamicLight.h"

USING_NS_CC;

DynamicLight::~DynamicLight()
{
    CC_SAFE_RELEASE(shadowRenderShader);
    CC_SAFE_RELEASE(shadowMapShader);
    CC_SAFE_RELEASE(occlusionMap);
    CC_SAFE_RELEASE(shadowMap1D);
    CC_SAFE_RELEASE(finalShadowMap);
    CC_SAFE_RELEASE(shadowCasters);
}

bool DynamicLight::init()
{
    if (!Node::init()) {
        return false;
    }

    auto shadowMapShaderP = this->loadShader("shaders/pass.vsh", "shaders/shadowMap.fsh");
	auto shadowRenderShaderP = this->loadShader("shaders/pass.vsh", "shaders/shadowRender.fsh");

	shadowMapShader = GLProgramState::getOrCreateWithGLProgram(shadowMapShaderP);
	shadowRenderShader = GLProgramState::getOrCreateWithGLProgram(shadowRenderShaderP);

    initOcclusionMap();
    initShadowMap1D();
    initFinalShadowMap();
    bakedMapIsValid = false;

    return true;
}

GLProgram* DynamicLight::loadShader(const GLchar* vertexShader, const GLchar* fragmentShader)
{
	auto shader = ShaderCache::getInstance()->getGLProgram(fragmentShader);
	if (!shader) {
		shader = new GLProgram();
		shader->initWithVertexShaderFilename(vertexShader, fragmentShader);
		shader->bindAttribLocation(GLProgram::ATTRIBUTE_NAME_POSITION, GLProgram::VERTEX_ATTRIB_POSITION);
		shader->bindAttribLocation(GLProgram::ATTRIBUTE_NAME_TEX_COORD, GLProgram::VERTEX_ATTRIB_TEX_COORDS);
		shader->bindAttribLocation(GLProgram::ATTRIBUTE_NAME_COLOR, GLProgram::VERTEX_ATTRIB_COLOR);
		shader->link();
		shader->updateUniforms();
		shader->use();
	}

	return shader;
}


void DynamicLight::initOcclusionMap()
{
    CC_SAFE_RELEASE(occlusionMap);
	CC_SAFE_RELEASE(occlusionMapSprite);

    occlusionMap = RenderTexture::create(lightSize, lightSize);
    occlusionMap->retain();

	occlusionMapSprite = Sprite::createWithTexture(occlusionMap->getSprite()->getTexture());
	occlusionMapSprite->retain();
}

void DynamicLight::initShadowMap1D()
{
    CC_SAFE_RELEASE(shadowMap1D);
	CC_SAFE_RELEASE(shadowMap1DSprite);

    // seems like 16 pixel is the minimum height of a texture (on ios)
    shadowMap1D = RenderTexture::create(lightSize, 16);
    shadowMap1D->retain();

	shadowMap1DSprite = Sprite::createWithTexture(shadowMap1D->getSprite()->getTexture());
	shadowMap1DSprite->retain();
}

void DynamicLight::initFinalShadowMap()
{
    CC_SAFE_RELEASE(finalShadowMap);
	CC_SAFE_RELEASE(finalShadowMapSprite);

    finalSize = lightSize * upScale;

    finalShadowMap = RenderTexture::create(finalSize, finalSize);
    finalShadowMap->retain();

	finalShadowMapSprite = Sprite::createWithTexture(finalShadowMap->getSprite()->getTexture());
	finalShadowMapSprite->retain();
}

void DynamicLight::setShadowCasters(Node* casters)
{
    CC_SAFE_RELEASE(shadowCasters);

    bakedMapIsValid = false;

    shadowCasters = Sprite::createWithTexture(dynamic_cast<Sprite*>(casters)->getTexture());
	shadowCasters->setAnchorPoint(casters->getAnchorPoint());
	shadowCasters->setPosition(casters->getPosition());
    shadowCasters->retain();
}

void DynamicLight::updateShadowMap(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, bool transformUpdated)
{
    
}

void DynamicLight::setPosition(const Point& position)
{
    if (position.x == getPosition().x && position.y == getPosition().y) {
        return;
    }

    Node::setPosition(position);

    ++updateCount;
    if (updateCount > updateFrequency) {
        updateCount = 0;
        bakedMapIsValid = false;
    }
}

void DynamicLight::draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags)
{
    if (!bakedMapIsValid) {
        bakedMapIsValid = true;

		updateUniforms();
		createShadowMap(renderer, transform, flags);

		// update shadowRenderShader textures
		shadowRenderShader->setUniformTexture("u_texture", occlusionMapSprite->getTexture());
		shadowRenderShader->setUniformTexture("u_texture2", shadowMap1DSprite->getTexture());

		finalShadowMapSprite->setColor({ 255, 255, 255 });
		finalShadowMapSprite->setGLProgramState(shadowRenderShader);
		finalShadowMapSprite->setAnchorPoint({ 0.5, 0.5 });
		finalShadowMapSprite->setPosition((-getPositionX() + lightSize / 2) / 2, (-getPositionY() + lightSize / 2) / 2);
		finalShadowMapSprite->setBlendFunc({ GL_SRC_COLOR , GL_ONE });
    }

	finalShadowMapSprite->visit(renderer, transform, flags);

    if (debugDrawEnabled) {
        debugDraw(renderer, transform, flags);
    }
}

void DynamicLight::debugDraw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, bool transformUpdated)
{
    auto glView = Director::getInstance()->getOpenGLView();
    auto width = glView->getDesignResolutionSize().width;
    auto height = glView->getDesignResolutionSize().height;

    auto occlusionX = width - lightSize / 2 - getPositionX();
    auto occlusionY = height - lightSize / 2 - getPositionY();

    auto shadowX = width - lightSize / 2 - getPositionX();
    auto shadowY = height - lightSize - 15 - getPositionY();

    occlusionMap->getSprite()->setColor(Color3B::RED);
    occlusionMap->setAnchorPoint({0, 0});
    occlusionMap->setPosition({occlusionX, occlusionY});
    occlusionMap->visit(renderer, transform, transformUpdated);
    occlusionMap->getSprite()->setColor(Color3B::WHITE);

    shadowMap1D->setAnchorPoint({0, 0});
    shadowMap1D->setPosition({shadowX, shadowY});
    shadowMap1D->visit(renderer, transform, transformUpdated);
}

void DynamicLight::updateUniforms()
{
	// update other uniforms
	shadowMapShader->setUniformVec2("resolution", Vec2(lightSize, lightSize));
	shadowMapShader->setUniformFloat("upScale", 1.0);
	shadowMapShader->setUniformFloat("accuracy", 1.0);

	shadowRenderShader->setUniformVec2("resolution", Vec2(lightSize, lightSize));
	shadowRenderShader->setUniformFloat("softShadows", softShadows ? 1.0f : 0.0f);
}

void DynamicLight::createOcclusionMap()
{
    
}

void DynamicLight::createShadowMap(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, bool transformUpdated)
{
	if (!shadowCasters) {
		occlusionMap->beginWithClear(0.0, 0.0, 0.0, 0.0);
		occlusionMap->end();
		return;
	}
	Point p1 = shadowCasters->getAnchorPoint();
	Point p2 = shadowCasters->getPosition();
	auto x = shadowCasters->getPositionX() - (getPositionX() - (lightSize / 2));
	auto y = shadowCasters->getPositionY() - (getPositionY() - (lightSize / 2));
	// Render light region to occluder FBO
	//occlusionMap->beginWithClear(255.0, 255.0, 255.0, 1.0);
	occlusionMap->beginWithClear(0, 0, 0, 0);
	shadowCasters->setPosition(x, y);
	shadowCasters->visit();
	occlusionMap->end();
	shadowCasters->setAnchorPoint(p1);
	shadowCasters->setPosition(p2);

	////////////////////////////////////////////////////////////////
	// set texture to shadow map shader
	occlusionMapSprite->setFlippedY(true);
	occlusionMapSprite->setAnchorPoint({ 0, 0 });
	occlusionMapSprite->setPosition({ -getPositionX(), -getPositionY() });
	shadowMapShader->setUniformTexture("u_texture", occlusionMapSprite->getTexture());
	occlusionMapSprite->setGLProgramState(shadowMapShader);

    // Build a 1D shadow map from occlude FBO
    shadowMap1D->beginWithClear(0.0, 0.0, 0.0, 0.0);
	occlusionMapSprite->visit(renderer, transform, transformUpdated);
    shadowMap1D->end();

}

void DynamicLight::setSoftShadows(bool shadows)
{
    if (softShadows != shadows) {
        softShadows = shadows;
        bakedMapIsValid = false;
    }
}

void DynamicLight::setLightSize(int lightSize)
{
    if (this->lightSize != lightSize) {
		if (lightSize < 0) lightSize = 0;
		this->lightSize = lightSize > 1200 ? 1200 : lightSize;
		initOcclusionMap();
		initShadowMap1D();
		initFinalShadowMap();

        bakedMapIsValid = false;
    }
}

void DynamicLight::setUpScale(float upScale)
{
    if (this->upScale != upScale) {
        this->upScale = upScale;
        bakedMapIsValid = false;
    }
}

void DynamicLight::setAccuracy(float accuracy)
{
    if (this->accuracy != accuracy) {
        this->accuracy = accuracy;
        bakedMapIsValid = false;
    }
}

void DynamicLight::setAdditive(bool additive)
{
    if (this->additive != additive) {
        this->additive = additive;
        bakedMapIsValid = false;
    }
}

void DynamicLight::setColor(const Color4B& color)
{
    if (this->color != color) {
        this->color = color;
        bakedMapIsValid = false;
    }
}
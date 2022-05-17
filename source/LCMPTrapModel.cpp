//
//  LCMPTrapModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPTrapModel.h"
#include <cugl/cugl.h>

using namespace std;
using namespace cugl;

//  MARK: - Constructors

/**
 * Initializes a Trap Model
 */
bool TrapModel::init(int trapID,
                     bool activated_,
                     const shared_ptr<physics2::PolygonObstacle> thiefArea, const shared_ptr<physics2::PolygonObstacle> copArea,
                     const shared_ptr<physics2::PolygonObstacle> triggerArea_, const shared_ptr<physics2::PolygonObstacle> deactivationArea_,
                     const shared_ptr<Vec2> triggerPosition,
                     bool copSolid, bool thiefSolid,
                     int numUses,
                     float copLingerDuration_,
                     float thiefLingerDuration_,
                     std::shared_ptr<Effect> copEffect_,
                     std::shared_ptr<Effect> thiefEffect_,
                     std::shared_ptr<Effect> copLingerEffect_,
                     std::shared_ptr<Effect> thiefLingerEffect_,
                     bool idleActivatedAnimation_,
                     bool idleDeactivatedAnimation_,
                     bool sfxOn, std::string sfxKey) {
    _trapID = trapID;
    activated = activated_;
    thiefEffectArea = thiefArea;
    copEffectArea = copArea;
    triggerArea = triggerArea_;
    deactivationArea = deactivationArea_;
    triggerPos = triggerPosition;
    copCollide = copSolid;
    thiefCollide = thiefSolid;
    usesRemaining = numUses;
    copLingerDuration = copLingerDuration_;
    thiefLingerDuration = thiefLingerDuration_;
    copEffect = copEffect_;
    thiefEffect = thiefEffect_;
    copLingerEffect = copLingerEffect_;
    thiefLingerEffect = thiefLingerEffect_;
    idleActivatedAnimation = idleActivatedAnimation_;
    idleDeactivatedAnimation = idleDeactivatedAnimation_;

    _sfxOn = sfxOn;
    _sfxKey = sfxKey;

    thiefEffectFilter = b2Filter();
    thiefEffectFilter.maskBits = 0b10000;
    thiefEffectFilter.categoryBits = 0b10000;
    thiefEffectArea->setFilterData(thiefEffectFilter);

    copEffectFilter = b2Filter();
    copEffectFilter.maskBits = 0b01000;
    copEffectFilter.categoryBits = 0b01000;
    copEffectArea->setFilterData(copEffectFilter);
    
    triggerFilter = b2Filter();
    triggerFilter.maskBits = 0b00100;
    triggerFilter.categoryBits = 0b00100;
    triggerArea->setFilterData(triggerFilter);

    deactivationFilter = b2Filter();
    deactivationFilter.maskBits = 0b01000;
    deactivationFilter.categoryBits = 0b01000;
    deactivationArea->setFilterData(deactivationFilter);

    return true;
}
/**
* Initializes an Effect
*/
bool TrapModel::Effect::init(TrapType type, std::shared_ptr<cugl::Vec2> effect) {
    traptype = type;
    effectVec = effect;

    return true;
}


//  MARK: - Methods

/**
 * Sets all of the assets for this trap
 */
void TrapModel::setAssets(Vec2 position, Vec2 dims,
                          float scale, Vec2 proportion, float tileSize,
    const std::shared_ptr<cugl::scene2::SceneNode>& node,
    const std::shared_ptr<cugl::AssetManager>& assets,
    const std::shared_ptr<cugl::Texture> activationTriggerTexture,
    const std::shared_ptr<cugl::Texture> deactivationTriggerTexture,
    const tuple <bool, int, int, std::string> assetInfo) {
    
    // Create nodes
    _activationTriggerNode = scene2::PolygonNode::allocWithTexture(activationTriggerTexture);
    _deactivationTriggerNode = scene2::PolygonNode::allocWithTexture(deactivationTriggerTexture);

    float effectWidth = thiefEffectArea->getSize().width;
    float effectHeight = thiefEffectArea->getSize().height;
    
    //there is a bug at this line for left and right animation assets... I have no clue why
    auto assetTexture = assets->get<Texture>(get<3>(assetInfo));
    if (get<0>(assetInfo)) {
        //texture is animated
        
        _assetNode = scene2::SpriteNode::alloc(assetTexture, get<1>(assetInfo), get<2>(assetInfo));
        _hasActivationAnimation = true;
//        Vec2 proportions = Vec2(effectWidth * get<2>(assetInfo) / assetTexture->getWidth(), effectHeight * get<1>(assetInfo) / assetTexture->getHeight()) * scale ;
        _assetNode->setScale(proportion * scale);
        //CULog("hello %f", _assetNode->getScale().x);

    }
    else {
        //texture is static
        _assetNode = scene2::SpriteNode::alloc(assetTexture, 1, 1);
        _hasActivationAnimation = false;
//        Vec2 proportions = Vec2(effectWidth / assetTexture->getWidth(), effectHeight / assetTexture->getHeight()) * scale;
        _assetNode->setScale(proportion * scale);
    }

    //Set the nodes' scales
    Vec2 activationScale_ = Vec2::ONE;//Vec2(activationTriggerTextureScale->x / activationTriggerTexture->getWidth(), activationTriggerTextureScale->y / activationTriggerTexture->getHeight());
    Vec2 deactivationScale_ = Vec2::ONE;//Vec2(deactivationTriggerTextureScale->x / deactivationTriggerTexture->getWidth(), deactivationTriggerTextureScale->x / deactivationTriggerTexture->getHeight());
    Vec2 effectAreaScale_ = Vec2(thiefEffectArea->getWidth() / assetTexture->getWidth(), thiefEffectArea->getHeight() / assetTexture->getHeight());

    // Set the nodes' positions
    _activationTriggerNode->setPosition((triggerPos->x /* + activationTriggerTextureScale->x / 2 */) * scale,
        (triggerPos->y /* + activationTriggerTextureScale->y / 2 */) * scale);
    _deactivationTriggerNode->setPosition((triggerPos->x /* + deactivationTriggerTextureScale->x/2 */) * scale,
        (triggerPos->y /* + deactivationTriggerTextureScale->y / 2 */) * scale);

    _assetNode->setPosition((position.x + dims.x / 2) * scale, (position.y + dims.y / 2) * scale);
//    CULog("trap asset node placed at %f %f", (position.x + dims.x / 2) * scale, (position.y + dims.y / 2) * scale);
    

    //CULog("%f, %f, %f, %f", triggerPos->x, triggerPos->y, thiefEffectArea->getPosition().x, thiefEffectArea->getPosition().y);

    // Set the nodes' scales
    _activationTriggerNode->setScale(PROP_SCALE*2);
    _deactivationTriggerNode->setScale(PROP_SCALE*2);
    //




    
    // TODO: these should really be children of a parent node that isn't the world node
    // Add the unactivated trigger node as the child
    _node = node;

    _assetNodeFrameCount = get<1>(assetInfo) * get<2>(assetInfo);
    if (!activated) {
        _node->addChild(_assetNode);
        _assetNode->setFrame(0);
        _node->addChild(_activationTriggerNode);
        //CULog("activated?");
    }
    else {
        _node->addChild(_deactivationTriggerNode);
        _node->addChild(_assetNode);
        _assetNode->setFrame(_assetNodeFrameCount-1);
    }
    

}

/**
 * Sets the debug scene to all of the children nodes
 */
void TrapModel::setDebugScene(const shared_ptr<scene2::SceneNode>& node) {
    thiefEffectArea->setDebugScene(node);
    copEffectArea->setDebugScene(node);
    triggerArea->setDebugScene(node);
    deactivationArea->setDebugScene(node);
}

/**
 * Returns true and decrements remaining uses if trap can be used. Returns false and has no effect otherwise.
 */
bool TrapModel::use() {
    if (usesRemaining < 0) return true;
    if (usesRemaining == 0) return false;
    usesRemaining--;
    return true;
}

/**
 * Begins the activate animation for traps
 */
void TrapModel::activate() {
    //CULog("activating");
    if (activated) return;

    if (_hasActivationAnimation) {
        activating = true;
    }
    else {
        activated = true;
        if (thiefCollide) {
            thiefEffectArea->setSensor(false);
        }
        if (copCollide) {
            copEffectArea->setSensor(false);
        }

        // Change which nodes are being shown
        _node->removeChild(_activationTriggerNode);
        _node->addChild(_deactivationTriggerNode);
    }

    
    //_node->addChild(_assetNode);
}


/**
 * Increments the activation animation until it's actually activated
 */
void TrapModel::updateTrap(float timestep) {
    if (idleActivatedAnimation) {
        prevTime += timestep;
        if (prevTime >= 0.1) {
            prevTime = 0;
            _assetNode->setFrame((_assetNode->getFrame() + 1) % _assetNodeFrameCount);
        }
    }
    //checks if the trap is in the middle of activating
    if (activating && _assetNode->getFrame() < _assetNodeFrameCount) {
        prevTime += timestep;
        if (prevTime >= 0.1) {
            prevTime = 0;
            _assetNode->setFrame(_assetNode->getFrame() + 1);
        }
        

        //once it has finished the animations, it will activate the trap and unset the flag to enter this loop
        if (_assetNode->getFrame() == _assetNodeFrameCount-1) {
            activated = true;
            if (thiefCollide) {
                thiefEffectArea->setSensor(false);
            }
            if (copCollide) {
                copEffectArea->setSensor(false);
            }

            // Change which nodes are being shown
            _node->removeChild(_activationTriggerNode);
            _node->addChild(_deactivationTriggerNode);
            //CULog("removing");

            activating = false;
        }
    }
    
}

/**
 * Deactivates this trap.
 */
void TrapModel::deactivate() {
    if (!activated) return;
    activated = false;
    thiefEffectArea->setSensor(true);
    copEffectArea->setSensor(true);
    
    //reset the animation to normal
    _assetNode->setFrame(0);

    // Change which nodes are being shown
    _node->removeChild(_deactivationTriggerNode);
    //_node->removeChild(_assetNode);
    _node->addChild(_activationTriggerNode);

   
}



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
                     const shared_ptr<physics2::SimpleObstacle> thiefArea, const shared_ptr<physics2::SimpleObstacle> copArea,
                     const shared_ptr<physics2::SimpleObstacle> triggerArea_, const shared_ptr<physics2::SimpleObstacle> deactivationArea_,
                     const shared_ptr<Vec2> triggerPosition,
                     bool copSolid, bool thiefSolid,
                     int numUses,
                     float copLingerDuration_,
                     float thiefLingerDuration_,
                     std::shared_ptr<Effect> copEffect_,
                     std::shared_ptr<Effect> thiefEffect_,
                     std::shared_ptr<Effect> copLingerEffect_,
                     std::shared_ptr<Effect> thiefLingerEffect_,
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
    _sfxOn = sfxOn;
    _sfxKey = sfxKey;

    activated = false;

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
void TrapModel::setAssets(float scale,
                          const std::shared_ptr<cugl::scene2::SceneNode>& node,
                          const std::shared_ptr<cugl::AssetManager>& assets,
                          const std::shared_ptr<cugl::Texture> activationTriggerTexture,
                          const std::shared_ptr<cugl::Texture> deactivationTriggerTexture,
                          const std::shared_ptr<cugl::Texture> unactivatedAreaTexture,
                          const std::shared_ptr<cugl::Texture> effectAreaTexture, 
                          const std::shared_ptr<Vec2> activationTriggerTextureScale,
                          const std::shared_ptr<Vec2> deactivationTriggerTextureScale,
                          const std::shared_ptr<Vec2> unactivatedAreaTextureScale,
                          const std::shared_ptr<Vec2> effectAreaTextureScale
) {
    
    // Create nodes
    _activationTriggerNode = scene2::PolygonNode::allocWithTexture(activationTriggerTexture);
    _deactivationTriggerNode = scene2::PolygonNode::allocWithTexture(deactivationTriggerTexture);
    _unactivatedAreaNode = scene2::PolygonNode::allocWithTexture(unactivatedAreaTexture);
    _effectAreaNode = scene2::PolygonNode::allocWithTexture(effectAreaTexture);

    //Set the nodes' scales
    Vec2 activationScale_ = Vec2(activationTriggerTextureScale->x / activationTriggerTexture->getWidth(), activationTriggerTextureScale->y / activationTriggerTexture->getHeight());
    Vec2 deactivationScale_ = Vec2(deactivationTriggerTextureScale->x / deactivationTriggerTexture->getWidth(), deactivationTriggerTextureScale->x / deactivationTriggerTexture->getHeight());
    Vec2 unactivatedAreaScale_ = Vec2(unactivatedAreaTextureScale->x / unactivatedAreaTexture->getWidth(), unactivatedAreaTextureScale->y / unactivatedAreaTexture->getHeight());
    Vec2 effectAreaScale_ = Vec2(effectAreaTextureScale->x / effectAreaTexture->getWidth(), effectAreaTextureScale->y / effectAreaTexture->getHeight());

    // Set the nodes' positions
    _activationTriggerNode->setPosition((triggerPos->x + activationTriggerTextureScale->x / 2) * scale,
        (triggerPos->y + activationTriggerTextureScale->y / 2) * scale);
    _deactivationTriggerNode->setPosition((triggerPos->x + deactivationTriggerTextureScale->x/2) * scale, 
        (triggerPos->y + deactivationTriggerTextureScale->y / 2) * scale);
    _unactivatedAreaNode->setPosition((thiefEffectArea->getPosition().x + unactivatedAreaTextureScale->x/2) * scale, 
        (thiefEffectArea->getPosition().y + unactivatedAreaTextureScale->y/2) * scale);
    _effectAreaNode->setPosition((thiefEffectArea->getPosition().x + effectAreaTextureScale->x/2) * scale,
        (thiefEffectArea->getPosition().y + effectAreaTextureScale->y / 2) * scale);

    //CULog("%f, %f, %f, %f", triggerPos->x, triggerPos->y, thiefEffectArea->getPosition().x, thiefEffectArea->getPosition().y);

    // Set the nodes' scales
    _activationTriggerNode->setScale(PROP_SCALE*2);
    _deactivationTriggerNode->setScale(PROP_SCALE*2);
    _unactivatedAreaNode->setScale(PROP_SCALE*2);
    _effectAreaNode->setScale(PROP_SCALE*2);

    
    // TODO: these should really be children of a parent node that isn't the world node
    // Add the unactivated trigger node as the child
    _node = node;

    if (!activated) {
        _node->addChild(_unactivatedAreaNode);
        _node->addChild(_activationTriggerNode);
        
    }
    else {
        _node->addChild(_deactivationTriggerNode);
        _node->addChild(_effectAreaNode);
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
 * Activates this trap.
 */
void TrapModel::activate() {
    //CULog("activating");
    if (activated) return;
    activated = true;
    if (thiefCollide) {
        thiefEffectArea->setSensor(false);
    }
    if (copCollide) {
        copEffectArea->setSensor(false);
    }

    
    // Change which nodes are being shown
    _node->removeChild(_activationTriggerNode);
    _node->removeChild(_unactivatedAreaNode);
    _node->addChild(_deactivationTriggerNode);
    _node->addChild(_effectAreaNode);
}

/**
 * Deactivates this trap.
 */
void TrapModel::deactivate() {
    if (!activated) return;
    activated = false;
    thiefEffectArea->setSensor(true);
    copEffectArea->setSensor(true);

    // Change which nodes are being shown
    _node->removeChild(_deactivationTriggerNode);
    _node->removeChild(_effectAreaNode);
    _node->addChild(_activationTriggerNode);
    _node->addChild(_unactivatedAreaNode);

   
}



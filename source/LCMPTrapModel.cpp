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
                     const std::shared_ptr<cugl::physics2::SimpleObstacle> area,
                     const std::shared_ptr<cugl::physics2::SimpleObstacle> triggerArea_,
                     const std::shared_ptr<cugl::Vec2> triggerPosition,
                     bool copSolid, bool thiefSolid,
                     int numUses,
                     float lingerDur,
                     const std::shared_ptr<cugl::Affine2> thiefVelMod,
                     const std::shared_ptr<cugl::Affine2> copVelMod,
                     bool startsTriggered) {
    _trapID = trapID;
    effectArea = area;
    triggerArea = triggerArea_;
    triggerPos = triggerPosition;
    copCollide = copSolid;
    thiefCollide = thiefSolid;
    usesRemaining = numUses;
    lingerDuration = lingerDur;
    thiefVelocityModifier = thiefVelMod;
    copVelocityModifier = copVelMod;

    effectFilter = b2Filter();
    effectFilter.maskBits = 0b010 + 0b10000*thiefCollide + 0b01000*copCollide;
    effectFilter.categoryBits = 0b010 + 0b10000 * thiefCollide + 0b01000 * copCollide;
    effectArea->setFilterData(effectFilter);
    
    triggerFilter = b2Filter();
    triggerFilter.maskBits = 0b00100;
    triggerFilter.categoryBits = 0b00100;
    triggerArea->setFilterData(triggerFilter);

    // TODO: This can cause some problems: if you init before setAssets, activate will be working with nodes that don't exist
    if (startsTriggered) activate();

    return true;
}

//  MARK: - Methods

/**
 * Sets all of the assets for this trap
 */
void TrapModel::setAssets(float scale,
                          const std::shared_ptr<cugl::scene2::SceneNode>& node,
                          const std::shared_ptr<cugl::AssetManager>& assets,
                          TrapType type) {
    // Get the textures
    _triggerTexture = assets->get<Texture>(getTriggerKey(type));
    _triggerActivatedTexture = assets->get<Texture>(getTriggerKey(type));
    _effectAreaTexture = assets->get<Texture>("tree");
    
    // Create nodes
    _triggerNode = scene2::PolygonNode::allocWithTexture(_triggerTexture);
    _triggerActivatedNode = scene2::PolygonNode::allocWithTexture(_triggerActivatedTexture);
    _effectAreaNode = scene2::PolygonNode::allocWithTexture(_effectAreaTexture);

    // Set the nodes' positions
    _triggerNode->setPosition(triggerPos->x * scale, triggerPos->y * scale);
    _triggerActivatedNode->setPosition(triggerPos->x * scale, triggerPos->y * scale);
    _effectAreaNode->setPosition(effectArea->getPosition() * scale);
    
    // TODO: these should really be children of a parent node that isn't the world node
    // Add the unactivated trigger node as the child
    _node = node;
    _node->addChild(_triggerNode);
}

/**
 * Sets the debug scene to all of the children nodes
 */
void TrapModel::setDebugScene(const shared_ptr<scene2::SceneNode>& node){
    effectArea->setDebugScene(node);
    triggerArea->setDebugScene(node);
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
void TrapModel::activate(){
    if (activated) return;
    activated = true;
    effectArea->setSensor(false);
    
    // Change which nodes are being shown
    _node->removeChild(_triggerNode);
    _node->addChild(_triggerActivatedNode);
    _node->addChild(_effectAreaNode);
}

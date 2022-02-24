//
//  LCMPThiefModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPThiefModel.h"

using namespace cugl;

void ThiefModel::setThiefNode(const std::shared_ptr<scene2::SceneNode>& node) {
    _thiefNode = node;
}

/**
 * Sets left/right movement of this character.
 *
 * This is the result of input times dude force.
 *
 * @param value left/right movement of this character.
 */
void ThiefModel::setMovement(Vec2 value) {
    if (value.length() > 1) {
        value.normalize();
    }
    _movement = value;
}

void ThiefModel::applyForce() {
    if (!isEnabled()) {
        return;
    }

    // Don't want to be moving. Damp out player motion
    if (getMovement() != Vec2::ZERO) {
        
    }
}

void ThiefModel::update(float delta) {
    CapsuleObstacle::update(delta);
    if (_thiefNode != nullptr) {
        _thiefNode->setPosition(getPosition() * _drawscale);
        _thiefNode->setAngle(getAngle());
    }
}
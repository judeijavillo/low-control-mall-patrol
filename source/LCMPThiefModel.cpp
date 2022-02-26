//
//  LCMPThiefModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPThiefModel.h"


#define MAX_SPEED       20000.0f
#define ACCELERATION    20000.0f

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
    if (value.lengthSquared() > 1) {
        value.normalize();
    }
    _movement = value;
}

void ThiefModel::applyForce() {
    if (!isEnabled()) {
        return;
    }

    Vec4 netforce((_movement.x * ACCELERATION), (_movement.y * ACCELERATION), 0.0f, 1.0f);
    Mat4::createRotationZ(getAngle(), &_affine);
    netforce *= _affine;
    // Don't want to be moving. Damp out player motion
    if (getMovement() == Vec2::ZERO || _body->GetLinearVelocity().Length() > MAX_SPEED) {
        netforce.set(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Apply force to the rocket BODY, not the rocket
    _body->ApplyForceToCenter(b2Vec2(netforce.x, netforce.y), true);
}

void ThiefModel::update(float delta) {
    CapsuleObstacle::update(delta);
    if (_thiefNode != nullptr) {
        _thiefNode->setPosition(getPosition() * _drawscale);
        _thiefNode->setAngle(getAngle());
    }
}
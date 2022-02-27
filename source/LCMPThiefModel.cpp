//
//  LCMPThiefModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPThiefModel.h"


#define MAX_SPEED       7.0f
#define ACCELERATION    4000000.0f

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
    if (getMovement() == Vec2::ZERO) {
        netforce.set(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Apply force to the thief
    _body->ApplyForceToCenter(b2Vec2(netforce.x, netforce.y), true);
     
    // Apply damping force to the thief.
    _body->ApplyForceToCenter(b2Vec2(_body-> GetLinearVelocity().x * (-ACCELERATION / MAX_SPEED), _body-> GetLinearVelocity().y * (-ACCELERATION / MAX_SPEED)), true);

    //Ensure thief does not travel faster than max speed.
    //if (_body->GetLinearVelocity().LengthSquared() > MAX_SPEED * MAX_SPEED) {
    //    Vec2 normalizedVelocity(_body->GetLinearVelocity().x, _body->GetLinearVelocity().y);
    //    normalizedVelocity.normalize();
    //    normalizedVelocity *= MAX_SPEED;
    //    _body->SetLinearVelocity(b2Vec2(normalizedVelocity.x, normalizedVelocity.y));
    //}
}

void ThiefModel::update(float delta) {
    CapsuleObstacle::update(delta);
    if (_thiefNode != nullptr) {
        _thiefNode->setPosition(getPosition() * _drawscale);
        _thiefNode->setAngle(getAngle());
        CULog("thief velocity: %f, %f", _body->GetLinearVelocity().x, _body->GetLinearVelocity().y);
    }
}

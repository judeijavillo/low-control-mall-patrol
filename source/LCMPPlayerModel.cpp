//
//  LCMPPlayerModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPPlayerModel.h"

/** The density of this player */
#define DEFAULT_DENSITY 1.0f
/** The friction of this player */
#define DEFAULT_FRICTION 0.4f
/** The restitution of this player */
#define DEFAULT_RESTITUTION 0.4f

using namespace cugl;

/**
 * initializes a Player Model
 */
bool PlayerModel::init(const Vec2 pos, const Size size, float scale,
                       const std::shared_ptr<cugl::scene2::SceneNode>& node) {
    // Call the parent's initializer
    physics2::CapsuleObstacle::init(pos, size);
    
    // Set physics properties for the body
    setName("player");
    setBodyType(b2_dynamicBody);
    setDensity(DEFAULT_DENSITY);
    setFriction(DEFAULT_FRICTION);
    setRestitution(DEFAULT_RESTITUTION);
    setFixedRotation(true);
    setDebugColor(Color4::RED);
    
    // Set collision sound
    _collisionSound  = "";
    
    // Save the scale (SCREEN UNITS / WORLD UNITS)
    _scale = scale;
    
    // Save player's top-level node
    _node = node;
    
    // Add a dropshadow node
    PolyFactory pf;
    Poly2 shadow = pf.makeCapsule(pos * scale, size * scale);
    _dropshadow = scene2::PolygonNode::allocWithPoly(shadow);
    _dropshadow->setAnchor(Vec2::ANCHOR_CENTER);
    _dropshadow->setPosition(Vec2::ZERO);
    _dropshadow->setColor(Color4(Vec4(0,0,0,0.25f)));
    _node->addChild(_dropshadow);
    
    // We're gonna assume this always works appropriately
    return true;
}

/**
 * Disposes of all resources in this instance of Player Model
 */
void PlayerModel::dispose() {
    _node = nullptr;
    _character = nullptr;
    _dropshadow = nullptr;
    
    _runBackTexture = nullptr;
    _runFrontTexture = nullptr;
    _runLeftTexture = nullptr;
    _runRightTexture = nullptr;
}

/**
 * Applies an acceleration to the player (most likely for local updates)
 */
void PlayerModel::applyForce(cugl::Vec2 force) {
    b2Vec2 b2force(force.x * getAcceleration(), force.y * getAcceleration());
    _body->ApplyForceToCenter(b2force, true);
    
    // Dampen the movement
    b2Vec2 b2velocity = _body->GetLinearVelocity();
    b2Vec2 b2damping(b2velocity.x * -getDamping(), b2velocity.y * -getDamping());
    _body->ApplyForceToCenter(b2damping, true);

    // If there is no input
    
    
    // If there is a non-negligible input
    if (force != Vec2::ZERO) {
        // Update the texture
        if (abs(force.x) >= abs(force.y)) {
            _character->setTexture(force.x > 0 ? _runRightTexture : _runLeftTexture);
        } else {
            _character->setTexture(force.y > 0 ? _runBackTexture : _runFrontTexture);
        }
    }
    
    // If the player has reached max speed
    //b2Vec2 b2velocity = _body->GetLinearVelocity();
    if (b2velocity.LengthSquared() >= getMaxSpeed() * getMaxSpeed()) {
        b2velocity.Normalize();
        b2velocity *= getMaxSpeed();
        _body->SetLinearVelocity(b2velocity);
    }
}

/**
 * Updates the position and velocity of the player (most likely for network updates)
 */
void PlayerModel::applyNetwork(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force) {
    setPosition(position);
    
    // Update the texture
    if (abs(force.x) >= abs(force.y)) {
        _character->setTexture(force.x > 0 ? _runRightTexture : _runLeftTexture);
    } else {
        _character->setTexture(force.y > 0 ? _runBackTexture : _runFrontTexture);
    }
}

/**
 * Updates the player node based on this player's body
 */
void PlayerModel::update(float timestep) {
    cugl::physics2::SimpleObstacle::update(timestep);
    
    if (_node != nullptr) {
        Vec2 position(_body->GetPosition().x, _body->GetPosition().y);
        _node->setPosition(position * _scale);
    }
}

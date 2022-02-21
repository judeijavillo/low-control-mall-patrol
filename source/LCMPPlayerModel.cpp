//
//  LCMPPlayerModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPPlayerModel.h"

using namespace cugl;

//  MARK: - Animation and Physics Constants

/** This is adjusted by screen aspect ratio to get the height */
#define GAME_WIDTH 1024

#define SIGNUM(x)  ((x > 0) - (x < 0))

// Default physics values
/** The density of this player */
#define DEFAULT_DENSITY 1.0f
/** The friction of this player */
#define DEFAULT_FRICTION 0.1f
/** The restitution of this player */
#define DEFAULT_RESTITUTION 0.4f
/** Height of the sensor attached to the player's feet */
#define SENSOR_HEIGHT   0.1f

//  MARK: - Constructors

bool PlayerModel::init(const Vec2 pos, const Size size) {
    physics2::CapsuleObstacle::init(pos,size);
    std::string name("player");
    setName(name);

    _playerNode = nullptr;
    setDensity(DEFAULT_DENSITY);
    setFriction(DEFAULT_FRICTION);
    setRestitution(DEFAULT_RESTITUTION);
    setFixedRotation(true);
    
    // Gameplay attributes
    _isGrounded = false;
    _faceRight  = true;

    return true;
}


/**
 * Disposes all resources and assets of this player
 */
void PlayerModel::dispose() {
    _playerNode = nullptr;
    _sensorNode = nullptr;
}

//  MARK: - Attribute Properties

/**
 * Sets left/right movement of this character.
 *
 * This is the result of input times dude force.
 *
 * @param value left/right movement of this character.
 */
void PlayerModel::setMovement(float value) {
    _movement = value;
    bool face = _movement > 0;
    if (_movement == 0 || _faceRight == face) {
        return;
    }
    
    // Change facing
    scene2::TexturedNode* image = dynamic_cast<scene2::TexturedNode*>(_playerNode.get());
    if (image != nullptr) {
        image->flipHorizontal(!image->isFlipHorizontal());
    }
    _faceRight = face;
}

//  MARK: - Physics
/**
 * Applies the force to the player
 *
 * This method should be called after the force attribute is set.
 */
void PlayerModel::applyForce() {
    if (!isEnabled()) {
        return;
    }
    
    // Don't want to be moving. Damp out player motion
    if (getMovement() == 0.0f) {
        if (isGrounded()) {
            // Instant friction on the ground
            b2Vec2 vel = _body->GetLinearVelocity();
            vel.x = 0; // If you set y, you will stop a jump in place
            _body->SetLinearVelocity(vel);
        } else {
            // Damping factor in the air
            b2Vec2 force(-getDamping()*getVX(),0);
            _body->ApplyForce(force,_body->GetPosition(),true);
        }
    }
    
    // Velocity too high, clamp it
    if (fabs(getVX()) >= getMaxSpeed()) {
        setVX(SIGNUM(getVX())*getMaxSpeed());
    } else {
        b2Vec2 force(getMovement(),0);
        _body->ApplyForce(force,_body->GetPosition(),true);
    }
}

/**
 * Updates the object's physics state (NOT GAME LOGIC).
 *
 * This method is called AFTER the collision resolution state. Therefore, it
 * should not be used to process actions or any other gameplay information.
 * Its primary purpose is to adjust changes to the fixture, which have to
 * take place after collision.
 *
 * In other words, this is the method that updates the scene graph.  If you
 * forget to call it, it will not draw your changes.
 *
 * @param delta Timing values from parent loop
 */
void PlayerModel::update(float delta) {
    CapsuleObstacle::update(delta);
    if (_playerNode != nullptr) {
        _playerNode->setPosition(getPosition()*_drawscale);
        _playerNode->setAngle(getAngle());
    }
}

//  MARK: - Animation


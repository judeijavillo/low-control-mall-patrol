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
    setBodyType(b2_dynamicBody);
    setDensity(DEFAULT_DENSITY);
    setFriction(DEFAULT_FRICTION);
    setRestitution(DEFAULT_RESTITUTION);
    setFixedRotation(true);
    
    // Gameplay attributes
    setAngle(0.0f);

    return true;
}


/**
 * Disposes all resources and assets of this player
 */
void PlayerModel::dispose() {
    _playerNode = nullptr;
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
    
    // OVERRIDE
}

void PlayerModel::setMovement(Vec2 value) {
    if (!isEnabled()) {
        return;
    }
    //OVERRIDE
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

/**
 * Sets the scene graph node representing this player.
 *
 * By storing a reference to the scene graph node, the model can update
 * the node to be in sync with the physics info. It does this via the
 * {@link Obstacle#update(float)} method.
 *
 * If the animation nodes are not null, this method will remove them from
 * the previous scene and add them to the new one.
 *
 * @param node  The scene graph node representing this player.
 */
void PlayerModel::setPlayerNode(const std::shared_ptr<scene2::SceneNode>&node) {
    _playerNode = node;
}


//  MARK: - Animation

/**
 * Sets the ratio of the ship sprite to the physics body
 *
 * The rocket needs this value to convert correctly between the physics
 * coordinates and the drawing screen coordinates.  Otherwise it will
 * interpret one Box2D unit as one pixel.
 *
 * All physics scaling must be uniform.  Rotation does weird things when
 * attempting to scale physics by a non-uniform factor.
 *
 * @param scale The ratio of the ship sprite to the physics body
 */
void PlayerModel::setDrawScale(float scale) {
    _drawscale = scale;
    if (_playerNode != nullptr) {
        _playerNode->setPosition(getPosition() * _drawscale);
    }
}

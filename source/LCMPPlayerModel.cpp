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
    
    // Create animations
    // Right character movement
    std::vector<int> east;
    for(int ii = 0; ii < 8; ii++) {
        east.push_back(ii);
    }
    _east = scene2::Animate::alloc(east,DURATION);

    // Left character movement
    std::vector<int> west;
    for(int ii = 0; ii < 8; ii++) {
        west.push_back(ii);
    }
    _west = scene2::Animate::alloc(west,DURATION);
    
    // We're gonna assume this always works appropriately
    return true;
}

/**
 * Disposes of all resources in this instance of Player Model
 */
void PlayerModel::dispose() {
    _node = nullptr;
    _dropshadow = nullptr;
    
    _characterLeft = nullptr;
    _characterRight = nullptr;
    _characterFront = nullptr;
    _characterBack = nullptr;
    
    runBack = nullptr;
    runFront = nullptr;
    runLeft = nullptr;
    runRight = nullptr;
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

/**
 * Performs a film strip action
 */
void PlayerModel::playAnimation(std::shared_ptr<scene2::ActionManager>& actions, Vec2 movement) {
    // Figure out which animation direction to use
    float angle = movement.getAngle(Vec2(1,0)) * 180 / 3.14;
    int key = (angle < 45 || angle >= -45) ? 0 : -1;
    if (angle < 135 && angle >= 45) key = 1;
    else if (angle >= 135 || angle < -135 || (movement.y == 0 && movement.x < 0)) key = 2;
    else if (angle < -45 && angle >= -135) key = 3;

    _characterRight->setVisible(false);
    _characterLeft->setVisible(false);
    _characterFront->setVisible(false);
    _characterBack->setVisible(false);
    // Animate different direction
    bool* cycle;
    std::shared_ptr<scene2::SpriteNode> node;
    switch (key) {
        case RIGHT_ANIM_KEY:
            actions->activate(ACT_KEY, _east, _characterRight);
            _characterRight->setVisible(true);
            node = _characterRight;
            cycle = &_rightCycle;
            break;
        case BACK_ANIM_KEY:
            actions->activate(ACT_KEY, _north, _characterBack);
            _characterBack->setVisible(true);
            node = _characterBack;
            cycle = &_backCycle;
            break;
        case LEFT_ANIM_KEY:
            actions->activate(ACT_KEY, _west, _characterLeft);
            _characterLeft->setVisible(true);
            node = _characterLeft;
            cycle = &_leftCycle;
            break;
        case FRONT_ANIM_KEY:
            actions->activate(ACT_KEY, _south, _characterFront);
            _characterFront->setVisible(true);
            node = _characterFront;
            cycle = &_frontCycle;
            break;
        default:
            break;
    }
    if (movement.length() > 0) {
        // Turn on the flames and go back and forth
        if (node->getFrame() == 0 || node->getFrame() == 1) {
            *cycle = true;
        } else if (node->getFrame() == node->getSize()-1) {
            *cycle = false;
        }

        // Increment
        if (*cycle) {
            node->setFrame(node->getFrame()+1);
        } else {
            node->setFrame(node->getFrame()-1);
        }
    } else {
        node->setFrame(0);
    }
}

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

//  MARK: - Constructors

/**
 * initializes a Player Model
 */
bool PlayerModel::init(int playerNumber, const Vec2 pos, const Size size, float scale,
                       const std::shared_ptr<cugl::scene2::SceneNode>& node,
                       std::shared_ptr<cugl::scene2::ActionManager>& actions) {
    // Call the parent's initializer
    physics2::CapsuleObstacle::init(pos, size);
    
    // Set physics properties for the body
    setBodyType(b2_dynamicBody);
    setDensity(DEFAULT_DENSITY);
    setFriction(DEFAULT_FRICTION);
    setRestitution(DEFAULT_RESTITUTION);
    setFixedRotation(true);
    setDebugColor(Color4::RED);
    
    // Save the player number
    _playerNumber = playerNumber;
    
    // Set collision sound
    _collisionSound  = "";
    
    // Save the scale (SCREEN UNITS / WORLD UNITS)
    _scale = scale;
    
    // Save player's top-level node
    _node = node;
    
    // Save action manager
    _actions = actions;
    
    // Add a dropshadow node
    PolyFactory pf;
    Poly2 shadow = pf.makeCapsule(pos * scale, size * scale);
    _dropshadow = scene2::PolygonNode::allocWithPoly(shadow);
    _dropshadow->setAnchor(Vec2::ANCHOR_CENTER);
    _dropshadow->setPosition(Vec2::ZERO);
    _dropshadow->setColor(Color4(Vec4(0,0,0,0.25f)));
    _node->addChild(_dropshadow);
    
    // Create animations
    for (int i = 0; i < _animFrames.size(); i++) {
        std::vector<int> vec;
        for(int ii = 0; ii < _animFrames[i]; ii++) {
            vec.push_back(ii);
        }
        _animations.push_back(scene2::Animate::alloc(vec,DURATION));
    }
    
    // We're gonna assume this always works appropriately
    return true;
}

/**
 * Disposes of all resources in this instance of Player Model
 */
void PlayerModel::dispose() {
    _node = nullptr;
    _dropshadow = nullptr;
    
    for (int i = 0; i < _spriteNodes.size(); i++) {
        _spriteNodes[i] = nullptr;
        _animations[i] = nullptr;
    }
}

//  MARK: - Methods

/**
 * Applies an acceleration to the player (most likely for local updates)
 */
void PlayerModel::applyForce(cugl::Vec2 force) {
    // Push the player in the direction they want to go
    b2Vec2 b2force(force.x * getAcceleration().x, force.y * getAcceleration().y);
    _realbody->ApplyForceToCenter(b2force, true);
    
    // Dampen the movement
    b2Vec2 b2velocity = _realbody->GetLinearVelocity();
    b2Vec2 b2damping(b2velocity.x * -getDamping().x, b2velocity.y * -getDamping().y);
    _realbody->ApplyForceToCenter(b2damping, true);
    
    // If the player has reached max speed
    if (b2velocity.LengthSquared() >= getMaxSpeed() * getMaxSpeed()) {
        b2velocity.Normalize();
        b2velocity *= getMaxSpeed();
        _realbody->SetLinearVelocity(b2velocity);
    }
    
    // Save the force for animations later
    _movement = force;

    //check for trap effects
    Vec2 playerVel;
    Vec2 addedVel;

    //iterate through the player effects and execute them
    for (auto it = playerEffects.begin(); it != playerEffects.end(); it++) {
        if (playerEffects.count(it->first) > 0 && playerEffects[it->first]->size() > 0) {
            std::tuple<TrapModel::TrapType, shared_ptr<Vec2>> elem = it->second->at(0);
            switch (std::get<0>(elem))
            {

            case TrapModel::TrapType::Directional_VelMod:
                playerVel = Vec2(b2velocity.x, b2velocity.y);
                addedVel = -0.5 * abs(playerVel.dot(Vec2(std::get<1>(elem)->x, std::get<1>(elem)->y))) * Vec2(std::get<1>(elem)->x, std::get<1>(elem)->y);
                _realbody->SetLinearVelocity(b2Vec2(b2velocity.x + addedVel.x, b2velocity.y + addedVel.y));
                break;
            case TrapModel::TrapType::Teleport:
                setPosition(Vec2(std::get<1>(elem)->x, std::get<1>(elem)->y));
                break;
            case TrapModel::TrapType::Moving_Platform:
                _realbody->SetLinearVelocity(b2Vec2(b2velocity.x + std::get<1>(elem)->x, b2velocity.y + std::get<1>(elem)->y));
                break;
            default:
                break;
            }
        }

    }

    /**
    if (std::get<0>(escalatorFlag)) {
        _realbody->SetLinearVelocity(b2Vec2(b2velocity.x + std::get<1>(escalatorFlag).x, b2velocity.y + std::get<1>(escalatorFlag).y));
    }

    if (std::get<0>(teleportFlag)) {
        setPosition(Vec2(std::get<1>(teleportFlag)));
    }

    if (std::get<0>(stairsFlag)) {
        Vec2 playerVel = Vec2(b2velocity.x, b2velocity.y);
        Vec2 addedVel = -0.5 * abs(playerVel.dot(std::get<1>(stairsFlag))) * std::get<1>(stairsFlag);
        CULog("%f", addedVel.x);
        _realbody->SetLinearVelocity(b2Vec2(b2velocity.x + addedVel.x, b2velocity.y + addedVel.y));
    }
    */
    // Save the force for animations later
    _movement = force;
}


/**
 * Adds the effect to the map of current player effects
 */
void PlayerModel::addEffects(int trapID, TrapModel::TrapType type, shared_ptr<cugl::Vec2> effect) {
    if (playerEffects.count(trapID) == 0) {
        playerEffects[trapID] = make_shared<vector<std::tuple<TrapModel::TrapType, shared_ptr<cugl::Vec2> >>>();
    }
    playerEffects[trapID]->push_back(make_tuple(type, effect));
}

/**
 * Removes an instance of the effect with the corresponding trap id from the player effects
 */
bool PlayerModel::removeEffects(int trapID) {
    if (playerEffects.count(trapID) ==0) {
        return false;
    }

    if (playerEffects[trapID]->size() == 1) {
        playerEffects.erase(trapID);
        return true;
    }

    playerEffects[trapID]->pop_back();
    return true;
}

/**
* applies the effect to the player model
*/
void PlayerModel::act(int trapID, std::shared_ptr<TrapModel::Effect> effect) {
    switch (effect->traptype)
    {
    case TrapModel::VelMod:
        setAccelerationMultiplier(Vec2(effect->effectVec->x, effect->effectVec->y));
        setMaxSpeedMultiplier(max(effect->effectVec->x, effect->effectVec->y));
        break;
    case TrapModel::Directional_VelMod:
        addEffects(trapID, effect->traptype, effect->effectVec);
        break;
    case TrapModel::Moving_Platform:
        addEffects(trapID, effect->traptype, effect->effectVec);
        break;
    case TrapModel::Teleport:
        addEffects(trapID, effect->traptype, effect->effectVec);
        break;
    default:
        break;
    }
}


/**
* reverts player model to default state, adding any linger effects as needed
*/
void PlayerModel::unact(int trapID, std::shared_ptr<TrapModel::Effect> effect) {
    switch (effect->traptype)
    {
    case TrapModel::VelMod:
        setAccelerationMultiplier(Vec2(1, 1));
        setMaxSpeedMultiplier(1);
        break;
    case TrapModel::Directional_VelMod:
        removeEffects(trapID);
        break;
    case TrapModel::Moving_Platform:
        removeEffects(trapID);
        break;
    case TrapModel::Teleport:
        removeEffects(trapID);
        break;
    default:
        break;
    }

    //TODO: Apply any lingering effects on the player
}

/**
 * Updates the position and velocity of the player (most likely for network updates)
 */
void PlayerModel::applyNetwork(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force) {
    setLinearVelocity(velocity);
    setPosition(position);
    applyForce(force);
}

/**
 * Updates the player node based on this player's body
 */
void PlayerModel::update(float timestep) {
    cugl::physics2::SimpleObstacle::update(timestep);
    
    if (_node != nullptr) {
        Vec2 position(_drawbody->GetPosition().x, _drawbody->GetPosition().y);
        _node->setPosition(position * _scale);
    }
}

/** Returns player animation key */
int PlayerModel::findDirection(Vec2 movement) {
    if (abs(movement.x) >= abs(movement.y)) {
        return (movement.x > 0 ? RIGHT_ANIM_KEY : LEFT_ANIM_KEY);
    } else {
        return (movement.y < 0 ? FRONT_ANIM_KEY : BACK_ANIM_KEY);
    }
}

/**
 * Performs a film strip action
 */
void PlayerModel::playAnimation() {
    // Figure out which animation direction to use
    int key = findDirection(_movement);
    
    // If there is no movement, use the still animation
    if (_movement.length() == 0) key = STILL_ANIM_KEY;
    
    // Play desired animation and pause others
    for (int i = 0; i < _spriteNodes.size(); i++) {
        shared_ptr<cugl::scene2::SpriteNode> s = _spriteNodes[i];
        string strKey = to_string(_playerNumber) + " " + to_string(i);
        if (i == key) {
            s->setVisible(true);
            if (_actions->isActive(strKey)) _actions->unpause(strKey);
            else _actions->activate(strKey, _animations[i], _spriteNodes[i]);
        }
        else {
            s->setVisible(false);
            _actions->pause(strKey);
        }
    }
}

/** Sets sprite nodes for animation */
void PlayerModel::setSpriteNodes(float width) {
    for (int i = 0; i < _spriteSheets.size(); i++) {
        _spriteNodes.push_back(scene2::SpriteNode::alloc(_spriteSheets[i], 1, _animFrames[i]));
        _spriteNodes[i]->setScale(CHAR_SCALE);
        _spriteNodes[i]->setAnchor(Vec2::ANCHOR_CENTER);
        _spriteNodes[i]->setPosition(Vec2(0, width / 2.5f));
        _spriteNodes[i]->setVisible(false);
        _node->addChild(_spriteNodes[i]);
    }
    _spriteNodes[STILL_ANIM_KEY]->setVisible(true);
}

//
//  LCMPGameScene.cpp
//  Network Lab
//
//  This class provides the main gameplay logic.
//
//  Author: Kevin Games
//  Version: 2/18/22
//

#include <cugl/cugl.h>
#include <iostream>
#include <sstream>
#include <box2d/b2_world.h>
#include <box2d/b2_contact.h>
#include <box2d/b2_collision.h>
#include <cugl/scene2/actions/CUActionManager.h>
#include <math.h>

#include "LCMPGameScene.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Whether or not to show the debug node */
#define DEBUG_ON        1

/** Width of the game world in Box2d units */
#define DEFAULT_WIDTH   32.0f
/** Height of the game world in Box2d units */
#define DEFAULT_HEIGHT  18.0f
/** The default value of gravity (going down) */
#define DEFAULT_GRAVITY 0.0f

#define TACKLE_COOLDOWN_TIME 1.5f
#define TACKLE_HIT_RADIUS 6.0f
#define TACKLE_ANGLE_MAX_ERR (M_PI_4)
#define TACKLE_LENGTH   50

#define RESET_TIME 5
#define SFX_TIME 5

/** The key for the floor tile */
#define TILE_TEXTURE    "floor"
/** The size for the floor tile */
#define TILE_SIZE       64

/** A coefficient for how much the camera tracks ahead of the player */
#define COP_LEAD_FACTOR     1.75f
/** A coefficient for how much the camera tracks ahead of the player */
#define THIEF_LEAD_FACTOR     1.75f
/** A coefficient for how much the camera interpolates between translations */
#define LERP_FACTOR     1.0f

/** A coefficient for how much smaller the text font should be */
#define TEXT_SCALAR         0.3f

/** The color for debugging */
Color4 DEBUG_COLOR =    Color4::YELLOW;

/** Counter for tackle */
int tackleCooldown = TACKLE_LENGTH;
/** Whether the cop has swiped recently */
bool onTackleCooldown = false;
/** Direction of the cop's tackle */
Vec2 tackleDir;
/** Position of the cop's node when the tackle is successful initially */
Vec2 copPosAtTackle;

/**
 * Returns true iff o1 should appear behind o2.
 */
bool compareNodes(shared_ptr<scene2::SceneNode> o1, shared_ptr<scene2::SceneNode> o2) {
    return o1->getPosition().y - o1->getHeight() / 2 > o2->getPosition().y - o2->getHeight() / 2;
}

//  MARK: - Constructors

/**
 * Initializes the controller contents, and starts the game
 *
 * The constructor does not allocate any objects or memory.  This allows
 * us to have a non-pointer reference to this controller, reducing our
 * memory allocation.  Instead, allocation happens in this method.
 *
 * @param assets    The (loaded) assets for this game mode
 *
 * @return true if the controller is initialized properly, false otherwise.
 */
bool GameScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
                     std::shared_ptr<NetworkController>& network,
                     std::shared_ptr<AudioController>& audio,
                     std::shared_ptr<cugl::scene2::ActionManager>& actions) {
    // Initialize the scene to a locked width
    _dimen = Application::get()->getDisplaySize();
    _screenSize = _dimen;
    _dimen *= SCENE_HEIGHT / _dimen.height;
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(_dimen)) return false;
    
    // Save the references to managers and controllers
    _assets = assets;
    _network = network;
    _audio = audio;
    _actions = actions;
    
    // Create the font
    _font = _assets->get<Font>("gyparody");
    
    // Initialize the input controller
    _input.init(getBounds());
    
    // Calculate the scale
    Rect rect(0,0,DEFAULT_WIDTH,DEFAULT_HEIGHT);
    Vec2 gravity(0,DEFAULT_GRAVITY);
    _offset = Vec2((_dimen.width-SCENE_WIDTH)/2.0f,(_dimen.height-SCENE_HEIGHT)/2.0f);
    _scale = _dimen.width == SCENE_WIDTH
        ? _dimen.width/rect.size.width
        : _dimen.height/rect.size.height;
    
    // Initialize the world
    _world = physics2::ObstacleWorld::alloc(rect,gravity);
    
    // Add callbacks for entering/leaving collisions
    _world->activateCollisionCallbacks(true);
    _world->onBeginContact = [this](b2Contact* contact) { _collision.beginContact(contact); };
    _world->onEndContact = [this](b2Contact* contact) { _collision.endContact(contact); };
    
    // Add callback for filtering collisions
    _world->activateFilterCallbacks(true);
    _world->shouldCollide = [this](b2Fixture* f1, b2Fixture* f2) { return _collision.shouldCollide(f1, f2); };

    // Initialize the floor
    _floornode = scene2::SceneNode::alloc();
    shared_ptr<Texture> tile = _assets->get<Texture>(TILE_TEXTURE);
    float length = tile->getWidth();
    float rows = ceil(SCENE_HEIGHT / TILE_SIZE);
    float cols = ceil(SCENE_WIDTH / TILE_SIZE);
    for (int i = -rows; i < rows * 2; i++) {
        for (int j = -cols; j < cols * 2; j++) {
            shared_ptr<scene2::PolygonNode> floortile = scene2::PolygonNode::allocWithTexture(tile);
            floortile->setScale(TILE_SIZE / length);
            floortile->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
            floortile->setPosition(Vec2(TILE_SIZE * j, TILE_SIZE * i));
            _floornode->addChild(floortile);
        }
    }
    _floornode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _floornode->setPosition(_offset);
    // TODO: Tile this in a better way. This is an expensive shortcut.
    
    // Create the world node
    _worldnode = scene2::SceneNode::alloc();
    _worldnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _worldnode->setPosition(_offset);
    
    // Create the debug node. Note: Debug node draws in PHYSICS coordinates
    _debugnode = scene2::SceneNode::alloc();
    _debugnode->setScale(_scale);
    _debugnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _debugnode->setPosition(_offset);
    _debugnode->setVisible(DEBUG_ON);
    
    // Create the UI node.
    _uinode = scene2::SceneNode::alloc();
    _uinode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _uinode->setPosition(_offset);
    
    // Add the nodes as children
    addChild(_floornode);
    addChild(_worldnode);
    addChild(_debugnode);
    addChild(_uinode);
    
    // Set some flags
    _quit = false;
    _gameover = false;
    _hitTackle = false;
    setActive(false);
    
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void GameScene::dispose() {
    if (_active) {
        removeAllChildren();
        _active = false;
    }
}

//  MARK: - Methods

/**
 * Sets whether the player is host.
 *
 * We may need to have gameplay specific code for host.
 *
 * @param host  Whether the player is host.
 */
void GameScene::start(bool host) {
    _ishost = host;
    _playerNumber = _network->getPlayerNumber();
    _isThief = (_playerNumber == -1);

    // Initialize the game
    _game = make_shared<GameModel>();
    _game->init(_world, _worldnode, _debugnode, _assets, _scale, LEVEL_ONE_FILE, _actions);
    
    // Initialize subcontrollers
    _collision.init(_game);
    _ui.init(_worldnode, _uinode, _game, _font, _screenSize, _offset);
}

/**
 * The method called to update the scene.
 *
 * We need to update this method to constantly talk to the server
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void GameScene::update(float timestep) {
    if (!_active) return;
    _gameTime += timestep;
    
    // Gameover updates
    _gameover = _game->isGameOver();
    if (_gameover) {
        _ui.update(timestep, _isThief, Vec2::ZERO, false, Vec2::ZERO, Vec2::ZERO, _playerNumber);
        if (_gameTime - _resetTime > RESET_TIME) {
            reset();
        }
        return;
    }
    
    // Input updates
    _input.update(timestep);
    Vec2 origin = _input.touch2Screen(_input.getJoystickOrigin());
    Vec2 position = _input.touch2Screen(_input.getJoystickPosition());
    Vec2 movement = _input.getMovementVector(_isThief);
    bool joystick = _input.didPressJoystick();
    bool spacebar = _input._spacebarPressed;
    
    // Swipe updates and animation
    if (!_isThief) {
        if (!_hitTackle) {
            _hitTackle = tackle(timestep, movement);
        }
        else {
            _resetTime = _gameTime;
            _game->setGameOver(successfulTackle(timestep));
            _network->sendGameOver();
        }
    }
    else {
        _game->getThief()->playAnimation(movement);
        _actions->update(timestep);
    }
    
    // Switch updates
    if (_input.didSwitch()) {
        // Stop movement
        movement = Vec2::ZERO;
        _isThief = !_isThief;
        _playerNumber = _playerNumber == -1 ? 0 : -1;
    }
    
    // Local updates
    shared_ptr<PlayerModel> player;
    Vec2 flippedMovement = Vec2(movement.x, -movement.y);
    if (_isThief) {
        int trapID = _game->getThief()->trapActivationFlag;
        if (spacebar && trapID != -1) {
            _game->activateTrap(trapID);
            _network->sendTrapActivation(trapID);
        }
        _game->updateThief(flippedMovement);
        _network->sendThiefMovement(_game, flippedMovement);
        player = _game->getThief();
    } else {
        _game->updateCop(flippedMovement, _playerNumber, onTackleCooldown);
        _network->sendCopMovement(_game, flippedMovement, _playerNumber);
        player = _game->getCop(_playerNumber);
    }

    // Floor updates
    Vec2 chunk = Vec2(floor(player->getPosition().x * _scale / SCENE_WIDTH),
                      floor(player->getPosition().y * _scale / SCENE_HEIGHT));
    _floornode->setPosition(chunk * Vec2(SCENE_WIDTH, SCENE_HEIGHT));
    
    // Network updates
    if (_network->isConnected()) _network->update(_game);
    
    // Camera updates
    float lead = _isThief ? THIEF_LEAD_FACTOR : COP_LEAD_FACTOR;
    Vec2 curr = _camera->getPosition();
    Vec2 next = _offset + player->getPosition() * _scale
        + (player->getVelocity() * _scale * lead);
    _camera->translate((next - curr) * timestep * LERP_FACTOR);
    _camera->update();
    
    // UI updates
    _ui.update(timestep, _isThief, movement, joystick, origin, position, _playerNumber);
    _uinode->setPosition(_camera->getPosition() - Vec2(SCENE_WIDTH, SCENE_HEIGHT)/2 - _offset);
    
    // Sort world node children
    std::vector<std::shared_ptr<scene2::SceneNode>> children = _worldnode->getChildren();
    sort(children.begin(), children.end(), compareNodes);
    _worldnode->removeAllChildren();
    for (std::shared_ptr<scene2::SceneNode> child : children) _worldnode->addChild(child);
    // TODO: Figure out how to do this using OrderedNode, this is __very__ bad
    
    // Update the physics, then move the game nodes accordingly
    _world->update(timestep);
    _game->update(timestep);
    _input.clear();
}

/**
 * Sets whether the scene is currently active
 *
 * This method should be used to toggle all the UI elements.  Buttons
 * should be activated when it is made active and deactivated when
 * it is not.
 *
 * @param value whether the scene is currently active
 */
void GameScene::setActive(bool value) {
    if (isActive() != value) {
        Scene2::setActive(value);
        if (value) {
            _quit = false;
        }
    }
}

/** Resets the game once complete */
void GameScene::reset() {
    _gameTime = 0;
    _resetTime = 0;
    _world->clear();
    _input.clear();
    
    // Clear the nodes
    _worldnode->removeAllChildren();
    _debugnode->removeAllChildren();
    _uinode->removeAllChildren();
    
    // Make a new game
    _game = make_shared<GameModel>();
    _game->init(_world, _worldnode, _debugnode, _assets, _scale, LEVEL_ONE_FILE, _actions);
    
    // Initialize subcontrollers
    _collision.init(_game);
    _ui.init(_worldnode, _uinode, _game, _font, _screenSize, _offset);
    
    _gameover = false;
    _hitTackle = false;
}

//  MARK: - Helpers

/** Handles cop tackle movement */
bool GameScene::tackle(float dt, Vec2 movement) {
    int copID = _playerNumber;
    if (_input.didSwipe() && !onTackleCooldown) {
        onTackleCooldown = true;
        _tackleTime = 0;
        tackleDir = _input.getSwipe();
        Vec2 copToThiefDist = (_game->getThief()->getPosition() - _game->getCop(copID)->getPosition());
        float angle = (abs(tackleDir.getAngle()) - abs(copToThiefDist.getAngle()));
        
        _game->getCop(copID)->showTackle(tackleDir,true);
        if (abs(angle) <= TACKLE_ANGLE_MAX_ERR && copToThiefDist.lengthSquared() < TACKLE_HIT_RADIUS * TACKLE_HIT_RADIUS)
            return true; 
        else {
            _game->getCop(copID)->failedTackle(_tackleTime, tackleDir);
            return false;
        }
    }
    else if (onTackleCooldown) {
        _tackleTime += dt;
        _game->getCop(copID)->failedTackle(_tackleTime, tackleDir);
        if (_tackleTime >= TACKLE_COOLDOWN_TIME) {
            onTackleCooldown = false;
            _game->getCop(copID)->hideTackle();
            _game->getCop(copID)->playAnimation(movement);
            _actions->update(dt);
        }
        else if (_tackleTime >= TACKLE_COOLDOWN_TIME / 2) {
            _game->getCop(copID)->showTackle(tackleDir, false);
        }
    }
    else {
        _game->getCop(copID)->hideTackle();
        _game->getCop(copID)->playAnimation(movement);
        _actions->update(dt);
    }
    return false; 
}

/** Handles a successful tackle */
bool GameScene::successfulTackle(float dt) {
    int copID = _playerNumber;
    copPosAtTackle = _game->getCop(copID)->getPosition();
    Vec2 thiefPos = _game->getThief()->getPosition();
    Vec2 diff = thiefPos - copPosAtTackle;
    _tackleTime += dt;
    _game->getCop(copID)->setPosition(copPosAtTackle + (diff * (_tackleTime / TACKLE_AIR_TIME)));
    return (_tackleTime >= TACKLE_AIR_TIME);
}

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
#include "LCMPConstants.h"

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

/** The amount of time that the game waits before reseting */
#define RESET_TIME 3

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
    _state = INIT;
    _quit = false;
    setActive(false);
    
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void GameScene::dispose() {
    if (_active) {
        _audio->stopMusic(GAME_MUSIC);
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
    _gameTime = 0;
    _doneTime = 0;
    _isThiefWin = false;
    _isHost = host;
    
    _audio->playSound(_assets, GAME_MUSIC, false, -1);

    _playerNumber = _network->getPlayerNumber();
    _isThief = (_playerNumber == -1);

    // Initialize the game
    _game = make_shared<GameModel>();
    _game->init(_world, _worldnode, _debugnode, _assets, _scale, LEVEL_ONE_FILE, _actions);
    
    // Initialize subcontrollers
    _collision.init(_game);
    _ui.init(_worldnode, _uinode, _game, _font, _screenSize, _offset, _assets);
    
    // Update the state of the game
    _state = GAME;
}

/**
 * The method called to update the scene.
 *
 * We need to update this method to constantly talk to the server
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void GameScene::update(float timestep) {
    // Don't update if this scene isn't active
    if (!_active) return;
    
    // Call the appropriate state helper for updates
    switch (_state) {
    case INIT:
        stateInit(timestep);
        break;
    case GAME:
        stateGame(timestep);
        break;
    case DONE:
        stateDone(timestep);
        break;
    }
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

/**
 * Resets the game
 */
void GameScene::reset() {
    _gameTime = 0;
    _doneTime = 0;
    _isThiefWin = false;
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
    _ui.init(_worldnode, _uinode, _game, _font, _screenSize, _offset, _assets);
    
    // Update the state of the game
    _state = GAME;
}

//  MARK: - States

/**
 * The update method for when we are in state INIT
 */
void GameScene::stateInit(float timestep) {
    // TODO: Here is where beginning countdown and stuff would go
}

/**
 * The update method for when we are in state GAME
 */
void GameScene::stateGame(float timestep) {
    // Keep track of time
    _gameTime += timestep;
    
    // Play last frame's sound effects
    if (_collision.didHitObstacle) {
        _audio->playSound(_assets, OBJ_COLLISION_SFX, true, _gameTime);
        _collision.didHitObstacle = false;
    }
    if (_collision.didHitTrap) {
        _audio->playSound(_assets, TRAP_COLLISION_SFX, true, _gameTime);
        _collision.didHitTrap = false;
    }
    
    // Gather the input commands
    _input.update(timestep);
    Vec2 origin = _input.touch2Screen(_input.getJoystickOrigin());
    Vec2 position = _input.touch2Screen(_input.getJoystickPosition());
    Vec2 movement = _input.getMovementVector(_isThief);
    Vec2 tackle = _input.getSwipe();
    bool joystick = _input.didPressJoystick();
    bool activate = _input._spacebarPressed;
    bool swipe = _input.didSwipe();
    bool toggle = _input.didSwitch();
    movement.y = -movement.y;
    tackle.y = -tackle.y;
    
    // If time surpasses the game length, thief wins
    if (_gameTime > GAME_LENGTH) {
        _game->setGameOver(true);
        if (_isHost) _network->sendGameOver();
        _isThiefWin = true;
        _state = DONE;
        updateUI(timestep, _isThief, movement, joystick, origin, position, _playerNumber);
        return;
    }
    
    // Toggle updates
    // TODO: Remove this dev command, either here, or in InputController, or both
    if (toggle) {
        movement = Vec2::ZERO;
        _isThief = !_isThief;
        _playerNumber = _playerNumber == -1 ? 0 : -1;
    }
    
    // Update the game in these steps
    updateLocal(timestep, movement, activate, swipe, tackle);
    updateNetwork(timestep);
    updateCamera(timestep);
    updateFloor(timestep);
    updateUI(timestep, _isThief, movement, joystick, origin, position, _playerNumber);
    updateOrder(timestep);
    
    
    // Update the physics, then move the game nodes accordingly
    _actions->update(timestep);
    _world->update(timestep);
    _game->update(timestep);
    _input.clear();
    
    // Detect transition to next state
    for (int i = 0; i < _game->numberOfCops(); i++) {
        if (_game->getCop(i)->getCaughtThief()) {
            _audio->playSound(_assets, THIEF_COLLISION_SFX, true, _gameTime);
            if (_isHost) _game->setGameOver(true);
        }
    }
    if (_game->isGameOver()) {
        if (_isHost) _network->sendGameOver();
        _state = DONE;
    }
}

/**
 * The update method for when we are in state DONE
 */
void GameScene::stateDone(float timestep) {
    // Keep track of time
    _doneTime += timestep;
    
    updateUI(timestep, _isThief, Vec2::ZERO, false, Vec2::ZERO, Vec2::ZERO, _playerNumber);
    
    // Detect transition to next state
    if (_doneTime >= RESET_TIME) {
        reset();
        _state = GAME;
    }
}

//  MARK: - Helpers

/**
 * Updates local players (own player and non-playing players)
 */
void GameScene::updateLocal(float timestep, Vec2 movement, bool activate,
                            float swipe, Vec2 tackle) {
    // Update non-playing players if host
    if (_isHost) {
        for (int i = 0; i < 5; i++) {
            int copID = _network->getPlayer(i).playerNumber;
            if (!_network->isPlayerConnected(i) && copID != -1 && copID != _playerNumber) {
                updateCop(timestep, copID, Vec2::ZERO, false, Vec2::ZERO);
            }
        }
    }
    
    // Update own player
    if (_isThief) updateThief(timestep, movement, activate);
    else updateCop(timestep, _playerNumber, movement, swipe, tackle);
    
}

/**
 * Updates and networks the thief and any actions it can perform
 */
void GameScene::updateThief(float timestep, Vec2 movement, bool activate) {
    // Update and network thief movement
    _game->updateThief(movement);
    _network->sendThiefMovement(_game, movement);
    
    // Activate and network traps
    int trapID = _game->getThief()->trapActivationFlag;
    if (activate && trapID != -1) {
        _game->activateTrap(trapID);
        _network->sendTrapActivation(trapID);
    }
}

/**
 * Updates and networks a cop and any actions it can perform
 */
void GameScene::updateCop(float timestep, int copID, Vec2 movement, bool swipe, Vec2 tackle) {
    // Get some reusable variables
    shared_ptr<CopModel> cop = _game->getCop(copID);
    Vec2 thiefPosition = _game->getThief()->getPosition();
    
    // Play tackling sounds
    if (cop->didTackle) {
        _audio->playSound(_assets, TACKLE_SFX, true, _gameTime);
        cop->didTackle = false;
    }
    if (cop->didLand) {
        _audio->playSound(_assets, LAND_SFX, true, _gameTime);
        cop->didLand = false;
    }
    
    // Update, animate, and network cop movement
    _game->updateCop(movement, thiefPosition, copID, timestep);
    _network->sendCopMovement(_game, movement, copID);
    
    // Attempt tackle if appropriate
    if (swipe && !cop->getTackling()) {
        cop->attemptTackle(thiefPosition, tackle);
    }
}

/**
 * Updates based on data received over the network
 */
void GameScene::updateNetwork(float timestep) {
    _network->update(_game);
    // TODO: Add stuff here for migrating host, connection status, etc.
}

/**
 * Updates camera based on the position of the controlled player
 */
void GameScene::updateCamera(float timestep) {
    shared_ptr<PlayerModel> player = _isThief
        ? (shared_ptr<PlayerModel>) _game->getThief()
        : (shared_ptr<PlayerModel>) _game->getCop(_playerNumber);
    float lead = _isThief ? THIEF_LEAD_FACTOR : COP_LEAD_FACTOR;
    Vec2 curr = _camera->getPosition();
    Vec2 next = _offset
        + (player->getPosition() * _scale)
        + (player->getVelocity() * _scale * lead);
    _camera->translate((next - curr) * timestep * LERP_FACTOR);
    _camera->update();
}

/**
 * Updates the floor based on the camera
 */
void GameScene::updateFloor(float timestep) {
    Vec2 chunk = Vec2(floor(_camera->getPosition().x / SCENE_WIDTH),
                      floor(_camera->getPosition().y / SCENE_HEIGHT));
    _floornode->setPosition(chunk * Vec2(SCENE_WIDTH, SCENE_HEIGHT));
}

/**
 * Updates the UI and repositions the UI Node
 */
void GameScene::updateUI(float timestep, bool isThief, Vec2 movement,
                         bool didPress, Vec2 origin, Vec2 position, int playerNumber) {
    _ui.update(timestep, isThief, movement, didPress, origin, position, playerNumber, _gameTime, _isThiefWin);
    _uinode->setPosition(_camera->getPosition() - Vec2(SCENE_WIDTH, SCENE_HEIGHT)/2 - _offset);
}

/**
 * Updates the ordering of the nodes in the World Node
 */
void GameScene::updateOrder(float timestep) {
    std::vector<std::shared_ptr<scene2::SceneNode>> children = _worldnode->getChildren();
    sort(children.begin(), children.end(), compareNodes);
    _worldnode->removeAllChildren();
    for (std::shared_ptr<scene2::SceneNode> child : children) _worldnode->addChild(child);
    // TODO: Figure out how to do this using OrderedNode, this is __very__ bad
}

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
#include <math.h>

#include "LCMPGameScene.h"
#include "LCMPConstants.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Whether or not to show the debug node */
#define DEBUG_ON        1

/** This is the size of the active portion of the screen */
#define SCENE_WIDTH     1024
/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT    576
/** Amount UI elements should be shifted down to remain on screen */
#define SCENE_HEIGHT_ADJUST 80

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

/** How far from the thief the directional indicators appear. */
#define DIREC_INDICATOR_DIST_SCALAR     250.0f
/** How long one side of the directional indicator is. */
#define DIREC_INDICATOR_SIZE            60.0f

/** The radius of the outer accelerometer visualization. */
#define OUTER_ACCEL_VIS_RADIUS    75.0f
/** The radius of the inner accelerometer visualization. */
#define INNER_ACCEL_VIS_RADIUS    7.0f

/** A coefficient for how much smaller the text font should be */
#define TEXT_SCALAR         0.3f

/** The resting position of the joystick */
float JOYSTICK_HOME[]   {200, 200};

/**
 *  The position on the screen of the accelerometer visualization.
 *  The floats represent what fraction of the screen the x and y coordinates are at.
*/
float OUTER_ACCEL_VIS_POS[2]{ 0.1f, 0.1f };


// TODO: Factor out hard-coded starting positions
/** The positions of different trees */
float TREE_POSITIONS[2][2] = {{-10, 10}, {10, 10}};
/** The positions of different bushes */
float BUSH_POSITIONS[2][2] = {{-12, 2}, {12, 2}};
/** The positions of different bushes */
float FARIS_POSITION[2] = {0, 20};

/** The color for debugging */
Color4 DEBUG_COLOR =    Color4::YELLOW;

/** Counter for reset */
int resetTime = 0;
/** Counter for tackle */
int tackleCooldown = TACKLE_LENGTH;
/** Whether the cop has swiped recently */
bool onTackleCooldown = false;
/** Time since failed tackle */
float tackleTimer;
/** Direction of the cop's tackle */
Vec2 tackleDir;
/** Position of the cop's node when the tackle is successful initially */
Vec2 copPosAtTackle;

/**
 * Returns true iff o1 should appear behind o2.
 */
bool compareNodes(std::shared_ptr<scene2::SceneNode> o1, std::shared_ptr<scene2::SceneNode> o2) {
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
                     std::shared_ptr<AudioController>& audio) {
    // Initialize the scene to a locked width
    _dimen = Application::get()->getDisplaySize();
    _screenSize = _dimen;
    _dimen *= SCENE_HEIGHT/ _dimen.height;
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(_dimen)) return false;
    
    // Save the asset manager
    _assets = assets;
    
    // Save the network controller
    _network = network;
    
    // Save the audio controller
    _audio = audio;
    
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
    _world->onBeginContact = [this](b2Contact* contact) { beginContact(contact); };
    _world->onEndContact = [this](b2Contact* contact) { endContact(contact); };
    
    // Add callback for filtering collisions
    _world->activateFilterCallbacks(true);
    _world->shouldCollide = [this](b2Fixture* f1, b2Fixture* f2) { return shouldCollide(f1, f2); };

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
    
    // Create the font
    _font = _assets->get<Font>("gyparody");
    
    // Add text to the center of the UI node
    shared_ptr<scene2::SceneNode> message = scene2::Label::allocWithText("Cops Win!", _font);
    message->setAnchor(Vec2::ANCHOR_CENTER);
    message->setPosition(Vec2(SCENE_WIDTH/2,SCENE_HEIGHT/2) + _offset);
    message->setName("message");
    message->setVisible(false);
    _uinode->addChild(message);
    
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
    _isThief = host;
    // TODO: The host should not always be the thief
    
    // Initialize the game
    _game = make_shared<GameModel>();
    _game->init(_world, _worldnode, _debugnode, _assets, _scale, LEVEL_ONE_FILE);
    if (auto pID = _network->getPlayerID(); pID) {
        _playerID = (int)(*pID) - 1;
    } else {
        CULog("Failed to receive player ID");
    }
    
    // Call helpers
    initModels();
    initJoystick();
    initAccelVis();
    initDirecIndicators();
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
    if (_gameover) {
        resetTime++;
        if (resetTime > 120) {
            resetTime = 0;
            reset();
        }
        return;
    }
    
    // Input updates
    _input.update(timestep);
    Vec2 origin = _input.touch2Screen(_input.getJoystickOrigin());
    Vec2 position = _input.touch2Screen(_input.getJoystickPosition());
    Vec2 movement = _input.getMovementVector(_isThief);
    Vec2 difference = position - origin;
    bool spacebar = _input._spacebarPressed;
    
    // Swipe updates
    if (!_isThief) {
        if (!_hitTackle) {
            _hitTackle = tackle(timestep);
        }
        else {
            _gameover = successfulTackle(timestep);
        }
        
    }
    else {
        // Joystick updates
        if (_input.didPressJoystick()) {
            if (difference.length() > JOYSTICK_RADIUS)
                position = origin + difference.getNormalization() * JOYSTICK_RADIUS;
            _innerJoystick->setPosition(_input.didPressJoystick() ? position : JOYSTICK_HOME);
            _outerJoystick->setPosition(_input.didPressJoystick() ? origin : JOYSTICK_HOME);
            _outerJoystick->setVisible(true);
            _innerJoystick->setVisible(true);
        }
        else {
            _outerJoystick->setVisible(false);
            _innerJoystick->setVisible(false);
        }
    }
    
    // Switch updates
    if (_input.didSwitch()) {
        // Stop movement
        movement = Vec2::ZERO;
        _outerJoystick->setVisible(false);
        _innerJoystick->setVisible(false);
        
        // Remove indicator UI elements
        if (_isThief) {
            for (int i = 0; i < _game->numberOfCops(); i++) {
        //        _uinode->removeChild(_copDistances[i]);
                _uinode->removeChild(_direcIndicators[i]);
            }
        }
        else {
            _uinode->removeChild(_thiefDistance);
            initDirecIndicators();
        }
        _isThief = !_isThief;
    }
    
    // Local updates
    shared_ptr<PlayerModel> player;
    Vec2 flippedMovement = Vec2(movement.x, -movement.y);
    if (_isThief) {
        if (spacebar && _game->getThief()->trapActivationFlag != -1) {
            _trap->activate();
        }
        _game->updateThief(flippedMovement);
        _network->sendThiefMovement(_game, flippedMovement);
        player = _game->getThief();
    } else {
        _game->updateCop(flippedMovement, _playerID, onTackleCooldown);
        _network->sendCopMovement(_game, flippedMovement, _playerID);
        player = _game->getCop(_playerID);
    }
    
    shared_ptr<Font> font = _assets->get<Font>("gyparody");
    if (! _isThief) {
        // Calculate distance of thief from cop
        float distance = _game->getThief()->getPosition().distance(_game->getCop(0)->getPosition());
        // Create and show distance on screen
        _uinode->removeChildByName("thiefDistance");
        _thiefDistance = scene2::Label::allocWithText("Thief Distance: " + to_string(int(distance)), font);
        _thiefDistance->setAnchor(Vec2::ANCHOR_CENTER);
        _thiefDistance->setPosition(Vec2(SCENE_WIDTH/2,SCENE_HEIGHT-SCENE_HEIGHT_ADJUST) + _offset);
        _thiefDistance->setName("thiefDistance");
        _thiefDistance->setVisible(!_isThief);
        _uinode->addChild(_thiefDistance);
    }

    // Floor updates
    Vec2 chunk = Vec2(floor(player->getPosition().x * _scale / SCENE_WIDTH),
                      floor(player->getPosition().y * _scale / SCENE_HEIGHT));
    _floornode->setPosition(chunk * Vec2(SCENE_WIDTH, SCENE_HEIGHT));
    
    // Camera updates
    float lead = _isThief ? THIEF_LEAD_FACTOR : COP_LEAD_FACTOR;
    Vec2 curr = _camera->getPosition();
    Vec2 next = _offset + player->getPosition() * _scale
        + (player->getVelocity() * _scale * lead);
    _camera->translate((next - curr) * timestep * LERP_FACTOR);
    _camera->update();
    _uinode->setPosition(_camera->getPosition() - Vec2(SCENE_WIDTH, SCENE_HEIGHT)/2 - _offset);
    
    // Network updates
    if (_network->isConnected()) {
        _network->update(_game);
    }
    
    // Directional indicator updates
    updateDirecIndicators(_isThief);

    //Update accelerometer visualization for the cop.
    updateAccelVis(_isThief, flippedMovement);

    // Sort world node children
    std::vector<std::shared_ptr<scene2::SceneNode>> children = _worldnode->getChildren();
    sort(children.begin(), children.end(), compareNodes);
    _worldnode->removeAllChildren();
    for (std::shared_ptr<scene2::SceneNode> child : children) _worldnode->addChild(child);
    // TODO: Figure out how to do this using OrderedNode, this is __very__ bad
    
    
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
    _world->clear();
    _worldnode->removeAllChildren();
    _input.clear();
    _game = make_shared<GameModel>();
    _game->init(_world, _worldnode, _debugnode, _assets, _scale, LEVEL_ONE_FILE);
    initModels();
    _uinode->getChildByName("message")->setVisible(false);
    _gameover = false;
    _hitTackle = false;
}

//  MARK: - Helpers

/**
 * Creates the necessary nodes for showing the joystick and adds them to the UI node
 */
void GameScene::initJoystick() {
    // Create outer part of joystick
    _outerJoystick = scene2::PolygonNode::allocWithPoly(_pf.makeCircle(Vec2(0,0), JOYSTICK_RADIUS));
    _outerJoystick->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _outerJoystick->setScale(1.0f);
    _outerJoystick->setColor(_joystickColor);
    _outerJoystick->setVisible(_isThief);
    _uinode->addChild(_outerJoystick);
    
    // Create inner part of joystick view
    _innerJoystick = scene2::PolygonNode::allocWithPoly(_pf.makeCircle(Vec2(0,0), JOYSTICK_RADIUS / 2));
    _innerJoystick->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _innerJoystick->setScale(1.0f);
    _innerJoystick->setColor(_joystickColor);
    _innerJoystick->setVisible(_isThief);
    _uinode->addChild(_innerJoystick);
    
    // Reposition the joystick components
    _outerJoystick->setPosition(JOYSTICK_HOME);
    _innerJoystick->setPosition(JOYSTICK_HOME);
    
    // Hide joystick
    _outerJoystick->setVisible(false);
    _innerJoystick->setVisible(false);
}

/**
 *  Creates the necessary nodes for the accelerometer visualization and
 *  adds them to the UI node.
*/
void GameScene::initAccelVis() {
    // Create outer part of accelerometer visualization
    Poly2 outerCircle = _pf.makeCircle(Vec2(OUTER_ACCEL_VIS_POS) * Vec2(SCENE_WIDTH, SCENE_HEIGHT), OUTER_ACCEL_VIS_RADIUS);
    _outerAccelVis = scene2::PolygonNode::allocWithPoly(outerCircle);
    _outerAccelVis->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _outerAccelVis->setScale(1.0f);
    _outerAccelVis->setColor(_outerAccelVisColor);
    _outerAccelVis->setVisible(!_isThief);
    _uinode->addChild(_outerAccelVis);

    // Create inner part of accelerometer visualization
    Poly2 innerCircle = _pf.makeCircle(Vec2(OUTER_ACCEL_VIS_POS), INNER_ACCEL_VIS_RADIUS);
    _innerAccelVis = scene2::PolygonNode::allocWithPoly(innerCircle);
    _innerAccelVis->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _innerAccelVis->setScale(1.0f);
    _innerAccelVis->setColor(_innerAccelVisColor);
    _innerAccelVis->setVisible(_isThief);
    _uinode->addChild(_innerAccelVis);
}

/**
 * Creates the player and trap models and adds them to the world node
 */
void GameScene::initModels() {
    
    // TODO: Begin Remove
    
//    // Create trees
//    for (int i = 0; i < 2; i++) {
//        // Create tree node
//        shared_ptr<Texture> treeTexture = _assets->get<Texture>("tree");
//        shared_ptr<scene2::PolygonNode> treeNode = scene2::PolygonNode::allocWithTexture(treeTexture);
//        treeNode->setAnchor(Vec2::ANCHOR_CENTER);
//        treeNode->setScale(0.75f);
//        _worldnode->addChild(treeNode);
//
//        // Create tree
//        std::shared_ptr<ObstacleModel> tree = std::make_shared<ObstacleModel>();
//        tree->init(_scale, treeTexture, ObstacleModel::TREE);
//        tree->setDebugScene(_debugnode);
//        _world->addObstacle(tree);
//
//        // Position tree afterwards to not have to deal with changing world size
//        tree->setPosition(Vec2(TREE_POSITIONS[i]));
//        treeNode->setPosition((Vec2(TREE_POSITIONS[i]) + Vec2(5, 5.5f)) * _scale);
//    }

//    // Create bushes
//    for (int i = 0; i < 2; i++) {
//        // Create bush node
//        shared_ptr<Texture> bushTexture = _assets->get<Texture>("bush");
//        shared_ptr<scene2::PolygonNode> bushNode = scene2::PolygonNode::allocWithTexture(bushTexture);
//        bushNode->setAnchor(Vec2::ANCHOR_CENTER);
//        bushNode->setScale(0.75f);
//        _worldnode->addChild(bushNode);
//
//        // Create bush
//        std::shared_ptr<ObstacleModel> bush = std::make_shared<ObstacleModel>();
//        bush->init(_scale, bushTexture, ObstacleModel::BUSH);
//        bush->setDebugScene(_debugnode);
//        _world->addObstacle(bush);
//
//        // Position bush afterwards to not have to deal with changing world size
//        bush->setPosition(Vec2(BUSH_POSITIONS[i]));
//        bushNode->setPosition((Vec2(BUSH_POSITIONS[i]) + Vec2(5, 2.5f)) * _scale);
//    }
    
//    // Create faris node
//    shared_ptr<Texture> farisTexture = _assets->get<Texture>("faris");
//    shared_ptr<scene2::PolygonNode> farisNode = scene2::PolygonNode::allocWithTexture(farisTexture);
//    farisNode->setAnchor(Vec2::ANCHOR_CENTER);
//    farisNode->setScale(0.75f);
//    _worldnode->addChild(farisNode);
//
//    // Create faris
//    std::shared_ptr<ObstacleModel> faris = std::make_shared<ObstacleModel>();
//    faris->init(_scale, farisTexture, ObstacleModel::FARIS);
//    faris->setDebugScene(_debugnode);
//    _world->addObstacle(faris);
//
//    // Position faris afterwards to not have to deal with changing world size
//    faris->setPosition(Vec2(FARIS_POSITION));
//    farisNode->setPosition((Vec2(FARIS_POSITION) + Vec2(5, 7)) * _scale);
    
    // Create hard-coded example trap
    _trap = std::make_shared<TrapModel>();
    
    // Create the parameters to create a trap
    Vec2 center = Vec2(30, 30);
    std::shared_ptr<cugl::physics2::SimpleObstacle> area = physics2::WheelObstacle::alloc(Vec2::ZERO, 5);
    std::shared_ptr<cugl::physics2::SimpleObstacle> triggerArea = physics2::WheelObstacle::alloc(Vec2::ZERO, 3);
    std::shared_ptr<cugl::Vec2> triggerPosition = make_shared<cugl::Vec2>(center);
    bool copSolid = false;
    bool thiefSolid = false;
    int numUses = 1;
    float lingerDur = 0.3;
    std::shared_ptr<cugl::Affine2> thiefVelMod = make_shared<cugl::Affine2>(1, 0, 0, 1, 0, 0);
    std::shared_ptr<cugl::Affine2> copVelMod = make_shared<cugl::Affine2>(1, 0, 0, 1, 0, 0);

    // Initialize a trap
    _trap->init(area,
                triggerArea,
                triggerPosition,
                copSolid, thiefSolid,
                numUses,
                lingerDur,
                thiefVelMod, copVelMod);
    
    // Configure physics
    _world->addObstacle(area);
    _world->addObstacle(triggerArea);
    area->setPosition(center);
    triggerArea->setPosition(center);
    triggerArea->setSensor(true);
    
    // Set the appropriate visual elements
    _trap->setAssets(_scale, _worldnode, _assets, TrapModel::MopBucket);
    _trap->setDebugScene(_debugnode);
    
    // TODO: End Remove
    
}


/**
 *  Creates and displays directional indicators for the thief that point towards the cops.
 *  These indicators are added to the UI node.
*/
void GameScene::initDirecIndicators() {
    // Calculate distances of cops from thief
    for (int i = 0; i < _game->numberOfCops(); i++) {
        Vec2 distance = _game->getThief()->getPosition() - _game->getCop(i)->getPosition();
        float displayDistance = distance.length();
        distance = distance.getNormalization() * DIREC_INDICATOR_DIST_SCALAR;
        
        // Create and display directional indicators
        Poly2 triangle = _pf.makeTriangle(Vec2::ZERO, Vec2(DIREC_INDICATOR_SIZE,0.0f), Vec2(DIREC_INDICATOR_SIZE /2, DIREC_INDICATOR_SIZE * 1.5));
        _direcIndicators[i] = scene2::PolygonNode::allocWithPoly(triangle);
        _direcIndicators[i]->setAnchor(cugl::Vec2::ANCHOR_CENTER);
        _direcIndicators[i]->setScale(1.0f);
        _direcIndicators[i]->setColor(Color4::RED);
        _direcIndicators[i]->setVisible(_isThief);
        _uinode->addChild(_direcIndicators[i]);
        
        // Create and show cop distances on screen
        //_copDistances[i] = scene2::Label::allocWithText(to_string(int(displayDistance)), _font);
        //_copDistances[i]->setAnchor(Vec2::ANCHOR_CENTER);
        //_copDistances[i]->setPosition(_direcIndicators[i]->getPosition());
        //_copDistances[i]->setScale(TEXT_SCALAR);
        //_copDistances[i]->setVisible(_isThief);
        //_uinode->addChild(_copDistances[i]);
    }
}

/**
 *
 * Updates directional indicators
 */
void GameScene::updateDirecIndicators(bool isThief) {
    if (!_isThief) return;

    //Constants and variables used throughout the code
    const float size_scalar_min = 0.25;
    const float size_scalar_max_dist = 90;
    const int color_opacity = 200;
    float size_scalar;
    Vec2 thiefPos = _worldnode->nodeToScreenCoords(_worldnode->getChildByName("thief")->getPosition());
    Color4 color = Color4::RED;

    //Run calculations for each directonal indicator
    for (int i = 0; i < _game->numberOfCops(); i++) {
        //Set if directional indicators are visible or not based off if cop is on the screen
        _direcIndicators[i]->setVisible(isThief);
        Vec2 copPos = _worldnode->nodeToScreenCoords(_worldnode->getChildByName("cop" + to_string(i))->getPosition());
        if ((0 < copPos.x) && (copPos.x <= _screenSize.getIWidth()) && (0 < copPos.y) && (copPos.y <= _screenSize.getIHeight())) {
            _direcIndicators[i]->setVisible(false);
        }

        //Calculate distance and angle from thief to cop
        Vec2 distance = _game->getCop(i)->getPosition() - _game->getThief()->getPosition();
        float angle = distance.getAngle();
        float displayDistance = distance.length();

        //Set size and color of directional indicators based on distance
        size_scalar = (size_scalar_max_dist - displayDistance) / size_scalar_max_dist;
        if (size_scalar < size_scalar_min) size_scalar = size_scalar_min;
        color.set((int)(255 * size_scalar), (int)(255 * (1 - size_scalar)), 0, color_opacity);

        //Make the vector's origin at the thief 
        distance = distance.getNormalization() * (copPos - thiefPos).length();
        distance.x += thiefPos.x;
        distance.y += _screenSize.getIHeight() - thiefPos.y;

        //Make sure all directional indicators are displaying on the screen
        if (distance.x >= _screenSize.getIWidth() - DIREC_INDICATOR_SIZE * size_scalar)
            distance.x = _screenSize.getIWidth() - DIREC_INDICATOR_SIZE * size_scalar;
        if (distance.x <= DIREC_INDICATOR_SIZE * size_scalar)
            distance.x = DIREC_INDICATOR_SIZE * size_scalar;
        if (distance.y >= _screenSize.getIHeight() - DIREC_INDICATOR_SIZE * size_scalar)
            distance.y = _screenSize.getIHeight() - DIREC_INDICATOR_SIZE * size_scalar;
        if (distance.y <= DIREC_INDICATOR_SIZE * size_scalar)
            distance.y = DIREC_INDICATOR_SIZE * size_scalar;

        /*
        magic line to make things work on mobile
        (I think it converts screen coords to... node coords or something)
        (We do it in init() to make things adjust to variable screens, idk)
        */
        distance *= SCENE_HEIGHT / _screenSize.height;

        //Set up directional indicators
        _direcIndicators[i]->setPosition(distance);
        _direcIndicators[i]->setAngle(angle + M_PI_2 + M_PI);
        _direcIndicators[i]->setScale(size_scalar);
        _direcIndicators[i]->setColor(color);
    }
    _uinode->setPosition(_camera->getPosition() - Vec2(SCENE_WIDTH, SCENE_HEIGHT) / 2 - _offset);
}

/** Updates the accelerometer visualization */
void GameScene::updateAccelVis(bool isThief, Vec2 movement) {
    _outerAccelVis->setVisible(!isThief);
    _innerAccelVis->setVisible(!isThief);
    _innerAccelVis->setPosition(_outerAccelVis->getPosition() + (movement * OUTER_ACCEL_VIS_RADIUS));
    _uinode->setPosition(_camera->getPosition() - Vec2(SCENE_WIDTH, SCENE_HEIGHT) / 2 - _offset);
}


bool GameScene::tackle(float dt) {
    int copID = 0;
    if (_input.didSwipe() && !onTackleCooldown) {
        onTackleCooldown = true;
        tackleTimer = 0;
        tackleDir = _input.getSwipe();
        Vec2 copToThiefDist = (_game->getThief()->getPosition() - _game->getCop(copID)->getPosition());
        CULog("tackleDir angle: %f, copToThiefDist angle: %f", tackleDir.getAngle(), copToThiefDist.getAngle());
        float angle = (abs(tackleDir.getAngle()) - abs(copToThiefDist.getAngle()));
        CULog("angle: %f", angle);
        if (abs(angle) <= TACKLE_ANGLE_MAX_ERR && copToThiefDist.lengthSquared() < TACKLE_HIT_RADIUS * TACKLE_HIT_RADIUS)
            return true; 
        else {
            _game->getCop(copID)->failedTackle(tackleTimer, tackleDir);
            return false;
        }
    }
    else if (onTackleCooldown) {
        tackleTimer += dt;
        _game->getCop(copID)->failedTackle(tackleTimer, tackleDir);
        if (tackleTimer >= TACKLE_COOLDOWN_TIME) onTackleCooldown = false;
    }
    return false; 
}

bool GameScene::successfulTackle(float dt) {
    int copID = 0;
    copPosAtTackle = _game->getCop(copID)->getPosition();
    Vec2 thiefPos = _game->getThief()->getPosition();
    Vec2 diff = thiefPos - copPosAtTackle;
    tackleTimer += dt;
    _game->getCop(copID)->setPosition(copPosAtTackle + (diff * (tackleTimer / TACKLE_AIR_TIME)));
    return (tackleTimer >= TACKLE_AIR_TIME);
}


//  MARK: - Callbacks

/**
 * Callback for when two obstacles in the world begin colliding
 */
void GameScene::beginContact(b2Contact* contact) {
    b2Body* body1 = contact->GetFixtureA()->GetBody();
    b2Body* body2 = contact->GetFixtureB()->GetBody();
    b2Body* thiefBody = _game->getThief()->getBody();
    
    // Check all of the cops
    for (int i = 0; i < 4; i++) {
        b2Body* copBody = _game->getCop(i)->getBody();
        if ((thiefBody == body1 && copBody == body2) ||
            (thiefBody == body2 && copBody == body1)) {
            // Play collision sound
            std::string sound = _isThief ? _game->getThief()->getCollisionSound() : _game->getCop(0)->getCollisionSound();
            _audio->playSound(_assets, sound);
            // Display UI win elements
            _gameover = true;
            _uinode->getChildByName("message")->setVisible(true);
        }
    }

    // Check all of the traps
    // TODO: Rewrite this code to iterate over traps in GameModel
    auto triggerBody = _trap->getTriggerArea()->getBody();
    auto effectBody = _trap->getEffectArea()->getBody();

    if (_trap->activated) {
        if ((thiefBody == body1 && effectBody == body2) ||
            (thiefBody == body2 && effectBody == body1)) {
            _gameover = true;
        }
    }
    else {
        if ((thiefBody == body1 && triggerBody == body2) ||
            (thiefBody == body2 && triggerBody == body1)) {
            _game->getThief()->trapActivationFlag = _trap->trapId;
        }
    }
}

/**
 * Callback for when two obstacles in the world end colliding
 */
void GameScene::endContact(b2Contact* contact) {
    b2Body* body1 = contact->GetFixtureA()->GetBody();
    b2Body* body2 = contact->GetFixtureB()->GetBody();
    b2Body* thiefBody = _game->getThief()->getBody();

    if (_game->getThief()->trapActivationFlag != -1) {

        //TODO: make sure this checks the appriate trap in the array once we are not hardcoding test traps
        b2Body* triggerBody = _trap->getTriggerArea()->getBody();

        if ((thiefBody == body1 && triggerBody == body2) ||
            (thiefBody == body2 && triggerBody == body1)) {
            _game->getThief()->trapActivationFlag = -1;
        }
    }
}

/**
 * Callback for determining if two obstacles in the world should collide.
 */
bool GameScene::shouldCollide(b2Fixture* f1, b2Fixture* f2) {
    const b2Filter& filterA = f1->GetFilterData();
    const b2Filter& filterB = f2->GetFilterData();

    return filterA.maskBits & filterB.maskBits;
}

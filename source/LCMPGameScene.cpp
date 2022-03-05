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

#include "LCMPGameScene.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Whether or not to show the debug node */
#define DEBUG_ON        0

/** This is the size of the active portion of the screen */
#define SCENE_WIDTH     1024
/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT    576

/** Width of the game world in Box2d units */
#define DEFAULT_WIDTH   32.0f
/** Height of the game world in Box2d units */
#define DEFAULT_HEIGHT  18.0f
/** The default value of gravity (going down) */
#define DEFAULT_GRAVITY 0.0f

/** The width of a cop in world units */
#define COP_WIDTH       2.0f
/** The height of a cop body (its dropshadow) in world units */
#define COP_HEIGHT      1.0f

/** The width of the thief body (its dropshadow) in world units */
#define THIEF_WIDTH     2.0f
/** The height of the thief body (its dropshadow) in world units */
#define THIEF_HEIGHT    1.0f

/** The key for the floor tile */
#define TILE_TEXTURE    "floor"
/** The size for the floor tile */
#define TILE_SIZE       64

/** A coefficient for how much the camera tracks ahead of the player */
#define LEAD_FACTOR     2.0f
/** A coefficient for how much the camera interpolates between translations */
#define LERP_FACTOR     1.0f

/** The radius of the joystick*/
float JOYSTICK_RADIUS = 100;
/** The resting position of the joystick */
float JOYSTICK_HOME[]   {200, 200};

// TODO: Factor out hard-coded starting positions
/** The starting position of the thief in Box2D coordinates */
float THIEF_START[2] =  {10, 9};
/** The starting position of the cop in Box2D coordinates */
float COP_START[4][2] = {{-17, 6}, {-17, 12}, {-11, 6}, {-11, 12}};
/** The positions of different trees */
float TREE_POSITIONS[2][2] = {{-10, 10}, {10, 10}};
/** The positions of different bushes */
float BUSH_POSITIONS[2][2] = {{-12, 2}, {12, 2}};
/** The positions of different bushes */
float FARIS_POSITION[2] = {0, 20};

/** The color for debugging */
Color4 DEBUG_COLOR =    Color4::YELLOW;

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
                     std::shared_ptr<NetworkController>& network) {
    // Initialize the scene to a locked width
    Size dimen = Application::get()->getDisplaySize();
    dimen *= SCENE_HEIGHT/dimen.height;
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // Save the asset manager
    _assets = assets;
    
    // Save the network controller
    _network = network;
    
    // Initialize the input controller
    _input.init(getBounds());
    
    // Calculate the scale
    Rect rect(0,0,DEFAULT_WIDTH,DEFAULT_HEIGHT);
    Vec2 gravity(0,DEFAULT_GRAVITY);
    _offset = Vec2((dimen.width-SCENE_WIDTH)/2.0f,(dimen.height-SCENE_HEIGHT)/2.0f);
    _scale = dimen.width == SCENE_WIDTH
        ? dimen.width/rect.size.width
        : dimen.height/rect.size.height;
    
    // Initialize the world
    _world = physics2::ObstacleWorld::alloc(rect,gravity);
    _world->activateCollisionCallbacks(true);
    _world->onBeginContact = [this](b2Contact* contact) { beginContact(contact); };
    _world->beforeSolve = [this](b2Contact* contact, const b2Manifold* oldManifold) { beforeSolve(contact,oldManifold); };
    
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
    
    // Add text to the center of the UI node
    shared_ptr<Font> font = _assets->get<Font>("gyparody");
    shared_ptr<scene2::SceneNode> message = scene2::Label::allocWithText("Cops Win!", font);
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
    
    // Call helpers
    initModels();
    initJoystick();
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
    if (_gameover) return;
    
    // Input updates
    _input.update(timestep);
    Vec2 origin = _input.touch2Screen(_input.getJoystickOrigin());
    Vec2 position = _input.touch2Screen(_input.getJoystickPosition());
    Vec2 movement = _input.getMovementVector(_isThief);
    Vec2 difference = position - origin;
    
    // Joystick updates
    if (difference.length() > JOYSTICK_RADIUS)
        position = origin + difference.getNormalization() * JOYSTICK_RADIUS;
    _innerJoystick->setPosition(_input.didPressJoystick() ? position : JOYSTICK_HOME);
    _outerJoystick->setPosition(_input.didPressJoystick() ? origin : JOYSTICK_HOME);
    _uinode->setPosition(_camera->getPosition() - Vec2(SCENE_WIDTH, SCENE_HEIGHT)/2 - _offset);
    
    // Local updates
    shared_ptr<PlayerModel> player;
    Vec2 flippedMovement = Vec2(movement.x, -movement.y);
    if (_isThief) {
        _game->updateThief(flippedMovement);
        _network->sendThiefMovement(_game, flippedMovement);
        player = _game->getThief();
    } else {
        _game->updateCop(flippedMovement, 0);
        _network->sendCopMovement(_game, flippedMovement, 0);
        player = _game->getCop(0);
    }
    
    // Floor updates
    Vec2 chunk = Vec2(floor(player->getPosition().x * _scale / SCENE_WIDTH),
                      floor(player->getPosition().y * _scale / SCENE_HEIGHT));
    _floornode->setPosition(chunk * Vec2(SCENE_WIDTH, SCENE_HEIGHT));
    
    // Camera updates
    Vec2 curr = _camera->getPosition();
    Vec2 next = _offset + player->getPosition() * _scale
        + (player->getVelocity() * _scale * LEAD_FACTOR);
    _camera->translate((next - curr) * timestep * LERP_FACTOR);
    _camera->update();
    
    // Network updates
    if (_network->isConnected()) {
        _network->update(_game);
    }
    
    // Sort world node children
    std::vector<std::shared_ptr<scene2::SceneNode>> children = _worldnode->getChildren();
    sort(children.begin(), children.end(), compareNodes);
    _worldnode->removeAllChildren();
    for (std::shared_ptr<scene2::SceneNode> child : children) _worldnode->addChild(child);
    // TODO: Figure out how to do this using OrderedNode, this is __very__ bad
    
    _world->update(timestep);
    _game->update(timestep);
    
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

//  MARK: - Helpers

/**
 * Creates the necessary nodes for showing the joystick and adds them to the UI node
 */
void GameScene::initJoystick() {
    // Create polyfactory and translucent gray
    PolyFactory pf;
    Color4 color(Vec4(0, 0, 0, 0.25));
    
    // Create outer part of joystick
    _outerJoystick = scene2::PolygonNode::allocWithPoly(pf.makeCircle(Vec2(0,0), JOYSTICK_RADIUS));
    _outerJoystick->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _outerJoystick->setScale(1.0f);
    _outerJoystick->setColor(color);
    _outerJoystick->setVisible(_isThief);
    _uinode->addChild(_outerJoystick);
    
    // Create inner part of joystick view
    _innerJoystick = scene2::PolygonNode::allocWithPoly(pf.makeCircle(Vec2(0,0), JOYSTICK_RADIUS / 2));
    _innerJoystick->setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _innerJoystick->setScale(1.0f);
    _innerJoystick->setColor(color);
    _innerJoystick->setVisible(_isThief);
    _uinode->addChild(_innerJoystick);
    
    // Reposition the joystick components
    _outerJoystick->setPosition(JOYSTICK_HOME);
    _innerJoystick->setPosition(JOYSTICK_HOME);
}

/**
 * Creates the player and trap models and adds them to the world node
 */
void GameScene::initModels() {
    // Create thief node
    std::shared_ptr<scene2::SceneNode> thiefNode = scene2::SceneNode::alloc();
    thiefNode->setAnchor(Vec2::ANCHOR_CENTER);
    _worldnode->addChild(thiefNode);
    
    // Create thief
    Size thiefSize(THIEF_WIDTH, THIEF_HEIGHT);
    std::shared_ptr<ThiefModel> thief = std::make_shared<ThiefModel>();
    thief->init(Vec2::ZERO, thiefSize, _scale, thiefNode, _assets);
    thief->setDebugScene(_debugnode);
    _world->addObstacle(thief);
    
    // Position thief afterwards to not have to deal with changing world size
    thief->setPosition(Vec2(THIEF_START));
    
    // Create cops
    Size copSize(COP_WIDTH, COP_HEIGHT);
    std::unordered_map<int, std::shared_ptr<CopModel>> cops;
    for (int i = 0; i < 4; i++) {
        // Create cop node
        std::shared_ptr<scene2::SceneNode> copNode = scene2::SceneNode::alloc();
        copNode->setAnchor(Vec2::ANCHOR_CENTER);
        _worldnode->addChild(copNode);
        
        // Create cop
        std::shared_ptr<CopModel> cop = std::make_shared<CopModel>();
        cop->init(Vec2::ZERO, copSize, _scale, copNode, _assets);
        cop->setDebugScene(_debugnode);
        _world->addObstacle(cop);
        
        // Position cop afterwards to not have to deal with changing world size
        cop->setPosition(Vec2(COP_START[i]));
        
        // Add the cop to the mapping of cops
        cops[i] = cop;
    }
    
    // TODO: Begin Remove
    
    // Create trees
    for (int i = 0; i < 2; i++) {
        // Create tree node
        shared_ptr<Texture> treeTexture = _assets->get<Texture>("tree");
        shared_ptr<scene2::PolygonNode> treeNode = scene2::PolygonNode::allocWithTexture(treeTexture);
        treeNode->setAnchor(Vec2::ANCHOR_CENTER);
        treeNode->setScale(0.75f);
        _worldnode->addChild(treeNode);
        
        // Create tree
        std::shared_ptr<ObstacleModel> tree = std::make_shared<ObstacleModel>();
        tree->init(_scale, treeTexture, ObstacleModel::TREE);
        tree->setDebugScene(_debugnode);
        _world->addObstacle(tree);
        
        // Position tree afterwards to not have to deal with changing world size
        tree->setPosition(Vec2(TREE_POSITIONS[i]));
        treeNode->setPosition((Vec2(TREE_POSITIONS[i]) + Vec2(5, 5.5f)) * _scale);
    }
    
    
    // Create bushes
    for (int i = 0; i < 2; i++) {
        // Create bush node
        shared_ptr<Texture> bushTexture = _assets->get<Texture>("bush");
        shared_ptr<scene2::PolygonNode> bushNode = scene2::PolygonNode::allocWithTexture(bushTexture);
        bushNode->setAnchor(Vec2::ANCHOR_CENTER);
        bushNode->setScale(0.75f);
        _worldnode->addChild(bushNode);
        
        // Create bush
        std::shared_ptr<ObstacleModel> bush = std::make_shared<ObstacleModel>();
        bush->init(_scale, bushTexture, ObstacleModel::BUSH);
        bush->setDebugScene(_debugnode);
        _world->addObstacle(bush);
        
        // Position bush afterwards to not have to deal with changing world size
        bush->setPosition(Vec2(BUSH_POSITIONS[i]));
        bushNode->setPosition((Vec2(BUSH_POSITIONS[i]) + Vec2(5, 2.5f)) * _scale);
    }
    
    // Create faris node
    shared_ptr<Texture> farisTexture = _assets->get<Texture>("faris");
    shared_ptr<scene2::PolygonNode> farisNode = scene2::PolygonNode::allocWithTexture(farisTexture);
    farisNode->setAnchor(Vec2::ANCHOR_CENTER);
    farisNode->setScale(0.75f);
    _worldnode->addChild(farisNode);
    
    // Create faris
    std::shared_ptr<ObstacleModel> faris = std::make_shared<ObstacleModel>();
    faris->init(_scale, farisTexture, ObstacleModel::FARIS);
    faris->setDebugScene(_debugnode);
    _world->addObstacle(faris);
    
    // Position faris afterwards to not have to deal with changing world size
    faris->setPosition(Vec2(FARIS_POSITION));
    farisNode->setPosition((Vec2(FARIS_POSITION) + Vec2(5, 7)) * _scale);
    
    // TODO: End Remove
    
    // Create traps
    std::vector<std::shared_ptr<TrapModel>> traps;
    
    // Initialize the game
    _game = make_shared<GameModel>();
    _game->init(thief, cops, traps);
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
            _gameover = true;
            _uinode->getChildByName("message")->setVisible(true);
        }
    }
}

/**
 * Callback for when two obstacles in the world end colliding
 */
void GameScene::beforeSolve(b2Contact* contact, const b2Manifold* oldManifold) {
    
}

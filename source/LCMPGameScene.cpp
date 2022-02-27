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

/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT  720

/** This is the size of the active portion of the screen */
#define SCENE_WIDTH 1024

/** Scalar to change cop size.*/
#define TEXTURE_SCALAR 0.1f

/** Width of the game world in Box2d units */
#define DEFAULT_WIDTH   32.0f
/** Height of the game world in Box2d units */
#define DEFAULT_HEIGHT  18.0f
/** The default value of gravity (going down) */
#define DEFAULT_GRAVITY 0.0f

/** The initial thief position */
float THIEF_POS[] = {0, DEFAULT_HEIGHT/2};
float COP_POS[] = { DEFAULT_WIDTH / 4, DEFAULT_HEIGHT / 4 };

/** The key for the thief texture in the asset manager */
#define THIEF_TEXTURE        "thief"

#define COP_TEXTURE         "cop_left"

//  MARK: - Physics Constants

// Physics constants for initialization
/** Density of non-crate objects */
#define BASIC_DENSITY       0.0f
/** Friction of non-crate objects */
#define BASIC_FRICTION      0.1f
/** Collision restitution for all objects */
#define BASIC_RESTITUTION   0.1f
/** Threshold for generating sound on collision */
#define SOUND_THRESHOLD     3

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
bool GameScene::init(const std::shared_ptr<cugl::AssetManager>& assets, std::shared_ptr<NetworkController>& network) {
    // Initialize the scene to a locked width
    Size dimen = Application::get()->getDisplaySize();
    dimen *= SCENE_HEIGHT/dimen.height;
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // Save the asset manager
    _assets = assets;
    _input.init(getBounds());

    // Save the network controller
    _network = network;
    
    Rect rect = Rect(0,0,DEFAULT_WIDTH,DEFAULT_HEIGHT);
    Vec2 gravity = Vec2(0,DEFAULT_GRAVITY);
    
    // Create the world and attach the listeners.
    _world = physics2::ObstacleWorld::alloc(rect,gravity);
    _world->activateCollisionCallbacks(true);
    _world->onBeginContact = [this](b2Contact* contact) {
        beginContact(contact);
    };
//    _world->beforeSolve = [this](b2Contact* contact, const b2Manifold* oldManifold) {
//        beforeSolve(contact,oldManifold);
//    };
    
    // IMPORTANT: SCALING MUST BE UNIFORM
    // This means that we cannot change the aspect ratio of the physics world
    // Shift to center if a bad fit
    _scale = dimen.width == SCENE_WIDTH ? dimen.width/rect.size.width : dimen.height/rect.size.height;
    Vec2 offset((dimen.width-SCENE_WIDTH)/2.0f,(dimen.height-SCENE_HEIGHT)/2.0f);

    // Create the scene graph
    _worldnode = scene2::SceneNode::alloc();
    _worldnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _worldnode->setPosition(offset);

    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("game");
    scene->setContentSize(dimen);
    scene->doLayout(); // Repositions the HUD
    
    addChild(_worldnode);

    _quit = false;

    populate();
    addChild(scene);
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
    _input.dispose();
}

//  MARK: - Gameplay Handling
void GameScene::reset() {
    _world->clear();
    _worldnode->removeAllChildren();
    _input.clear();
    populate();
}

void GameScene::populate() {
    std::shared_ptr<Texture> image = _assets->get<Texture>(COP_TEXTURE);
    // Create cop
        Vec2 copPos = ((Vec2)COP_POS);
        Size copSize(image->getSize().width / _scale,
            image->getSize().height / _scale);
        
        _cop = CopModel::alloc(copPos,copSize);
        _cop->setDrawScale(_scale * TEXTURE_SCALAR);
        _world->addObstacle(_cop);

        auto copNode = scene2::PolygonNode::allocWithTexture(image);
        copNode->setAnchor(Vec2::ANCHOR_CENTER);
        copNode->setPosition(_cop->getPosition() * _scale);
        copNode->setScale(Vec2(TEXTURE_SCALAR, TEXTURE_SCALAR));

        _cop->setCopNode(copNode);

        _worldnode->addChild(copNode);

        // Create thief
        image = _assets->get<Texture>(THIEF_TEXTURE);
        Vec2 thiefPos = ((Vec2)THIEF_POS);
        Size thiefSize = image->getSize() / (_scale * TEXTURE_SCALAR);

        _thief = ThiefModel::alloc(thiefPos, thiefSize);
        _thief->setDrawScale(_scale);
        _world->addObstacle(_thief);

        auto thiefNode = scene2::PolygonNode::allocWithTexture(image);
        thiefNode->setAnchor(Vec2::ANCHOR_CENTER);
        thiefNode->setScale(Vec2(TEXTURE_SCALAR, TEXTURE_SCALAR));
        thiefNode->setPosition(_thief->getPosition() * _scale );

        _thief->setThiefNode(thiefNode);

        _worldnode->addChild(thiefNode);
}

//  MARK: - Methods

/**
 * The method called to update the scene.
 *
 * We need to update this method to constantly talk to the server
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void GameScene::update(float timestep) {
    if (_network->isConnected()) {
        _network->update(_game);
    }

    // Get input from joystick
    _input.update(timestep);

    Vec2 movement = _input.getMovement();
    
    // Thief movement
    if (_isThief) {
        _thief->setMovement(movement);
        _thief->applyForce();

        CULog("Thief Position: (%f, %f)", _thief->getPosition().x, _thief->getPosition().y);
    }
    // Cop movement
    else {
        _cop->setFX(_input.getHorizontal() * _cop->getThrust());
        _cop->setFY(_input.getVertical() * _cop->getThrust());
        _cop->applyForce();
        
        CULog("accel: (%f)", _cop->getAcceleration());
        CULog("hor,vert: (%f, %f)", _input.getHorizontal(), _input.getVertical());
        CULog("Cop Position: (%f, %f)", _cop->getPosition().x, _cop->getPosition().y);
    }

    _world->update(timestep);
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

//  MARK: - Physics Handling
/**
 * Processes the start of a collision
 *
 * This method is called when we first get a collision between two objects.  We use
 * this method to test if it is the "right" kind of collision.  In particular, we
 * use it to test if we make it to the win door.
 *
 * @param  contact  The two bodies that collided
 */
void GameScene::beginContact(b2Contact* contact) {
    b2Body* body1 = contact->GetFixtureA()->GetBody();
    b2Body* body2 = contact->GetFixtureB()->GetBody();
}

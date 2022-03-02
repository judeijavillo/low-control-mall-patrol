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
#include "LCMPGameModel.h"
#include "LCMPLevelConstants.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Regardless of logo, lock the height to this */
#define SCENE_WIDTH 1280
#define SCENE_HEIGHT  720

/** Scalar to change cop size.*/
#define TEXTURE_SCALAR 0.1f

/** Width of the game world in Box2d units */
#define DEFAULT_WIDTH   32.0f
/** Height of the game world in Box2d units */
#define DEFAULT_HEIGHT  18.0f
/** The default value of gravity (going down) */
#define DEFAULT_GRAVITY 0.0f

/** The initial thief position */
//float THIEF_POS[] = {0, DEFAULT_HEIGHT/2};
float THIEF_POS[] = { DEFAULT_WIDTH / 4, DEFAULT_HEIGHT / 4 };
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
    
//    _game = assets->get<GameModel>(LEVEL_ONE_KEY);
    _game = GameModel::alloc(LEVEL_ONE_FILE);
    if (_game == nullptr) {
        CULog("Fail!");
        return false;
    }
    

    // IMPORTANT: SCALING MUST BE UNIFORM
    // This means that we cannot change the aspect ratio of the physics world
    // Shift to center if a bad fit
    _scale = dimen.width == SCENE_WIDTH ? dimen.width/DEFAULT_WIDTH : dimen.height/DEFAULT_HEIGHT;

    // Acquire the scene built by the asset loader and resize it the scene
//    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("game");
    //std::shared_ptr<scene2::SceneNode> scene = _game->getRootNode();
    //scene->setContentSize(dimen);
    //scene->doLayout(); // Repositions the HUD

    std::shared_ptr<physics2::ObstacleWorld> world = _game->getWorld();
    activateWorldCollisions(world);
    
    _scale = dimen.width == SCENE_WIDTH ? dimen.width / world->getBounds().getMaxX() :
        dimen.height / world->getBounds().getMaxY();
    Vec2 offset((dimen.width - SCENE_WIDTH) / 2.0f, (dimen.height - SCENE_HEIGHT) / 2.0f);

    // Create the scene graph
    
    // Create the nodes responsible for displaying the joystick.
    _jstickDeadzoneNode = scene2::PolygonNode::alloc();
    _jstickDeadzoneNode->SceneNode::setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _jstickDeadzoneNode->setScale(1.0f);
    _jstickDeadzoneNode->setVisible(false);

    _jstickRadiusNode = scene2::PolygonNode::alloc();
    _jstickRadiusNode->SceneNode::setAnchor(cugl::Vec2::ANCHOR_CENTER);
    _jstickRadiusNode->setScale(1.0f);
    _jstickRadiusNode->setVisible(false);

//    addChild(_rootnode);
    addChild(_jstickDeadzoneNode);
    addChild(_jstickRadiusNode);

    _quit = false;
    _isPanning = false;
    anchor.setZero();
    currPos.setZero();
    prevPos.setZero();
    gamePosition.setZero();

    _rootnode = std::dynamic_pointer_cast<scene2::ScrollPane>(_assets->get<scene2::SceneNode>("game"));
    _rootnode->setConstrained(false);
    _rootnode->applyZoom(2);
    _rootnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _rootnode->setPosition(offset);

    _game->setAssets(_assets);
    _game->setRootNode(_rootnode);

    _rootnode->setContentSize(dimen);
    
    _debugnode = scene2::SceneNode::alloc();
    _debugnode->setScale(_scale); // Debug node draws in PHYSICS coordinates
    _debugnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _debugnode->setPosition(offset);
    
    addChild(_rootnode);

    populate();
//    addChild(scene); POTENTIAL MERGE ISSUE

    setActive(false);
    return true;
}

/**
 * Activates world collision callbacks on the given physics world and sets the onBeginContact and beforeSolve callbacks
 *
 * @param world the physics world to activate world collision callbacks on
 */
void GameScene::activateWorldCollisions(const std::shared_ptr<physics2::ObstacleWorld>& world) {
    world->activateCollisionCallbacks(true);
    world->onBeginContact = [this](b2Contact* contact) {
        beginContact(contact);
    };
    world->beforeSolve = [this](b2Contact* contact, const b2Manifold* oldManifold) {
        beforeSolve(contact, oldManifold);
    };
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void GameScene::dispose() {
    if (_active) {
        removeAllChildren();
        _active = false;
        _game = nullptr;
        _rootnode = nullptr;

    }
    _input.dispose();
}

//  MARK: - Gameplay Handling
void GameScene::reset() {
    removeAllChildren();
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

        auto copNode = scene2::PolygonNode::allocWithTexture(image);
        copNode->setAnchor(Vec2::ANCHOR_CENTER);
        copNode->setPosition(_cop->getPosition() * _scale);
        copNode->setScale(Vec2(TEXTURE_SCALAR, TEXTURE_SCALAR));

        _cop->setCopNode(copNode);

        _game->addObstacle(_cop, copNode);

        // Create thief
        image = _assets->get<Texture>(THIEF_TEXTURE);
        Vec2 thiefPos = ((Vec2)THIEF_POS);
        Size thiefSize = image->getSize() / (_scale * TEXTURE_SCALAR);

        _thief = ThiefModel::alloc(thiefPos, thiefSize);
        _thief->setDrawScale(_scale);

        auto thiefNode = scene2::PolygonNode::allocWithTexture(image);
        thiefNode->setAnchor(Vec2::ANCHOR_CENTER);
        thiefNode->setScale(Vec2(TEXTURE_SCALAR, TEXTURE_SCALAR));
        thiefNode->setPosition(_thief->getPosition() * _scale );

        _thief->setThiefNode(thiefNode);

        _game->addObstacle(_thief, thiefNode);
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
    if (_game== nullptr) {
        return;
    }
    if (_network->isConnected()) {
        _network->update(_game);
        
        _game->getWorld()->update(timestep);

        moveScreen();

        activateWorldCollisions(_game->getWorld());
    }
    
        // Get input from joystick
        _input.update(timestep);
        if (_input.withJoystick()) {
            displayJoystick();
        }
        else {
            _jstickDeadzoneNode->setVisible(false);
            _jstickRadiusNode->setVisible(false);
        }

        Vec2 movement = _input.getMovement();
        
        // Thief movement
        if (_isThief) {
            _thief->setMovement(movement);
            _thief->applyForce();

    //        CULog("Thief Position: (%f, %f)", _thief->getPosition().x, _thief->getPosition().y);
        }
        // Cop movement
        else {
            _cop->setFX(_input.getHorizontal() * _cop->getThrust());
            _cop->setFY(_input.getVertical() * _cop->getThrust());
            _cop->applyForce();
            
    //        CULog("accel: (%f)", _cop->getAcceleration());
    //        CULog("hor,vert: (%f, %f)", _input.getHorizontal(), _input.getVertical());
    //        CULog("Cop Position: (%f, %f)", _cop->getPosition().x, _cop->getPosition().y);
        }

        _game->getWorld()->update(timestep);
}

void GameScene::moveScreen() { // For testing ONLY. Don't use this in game.
    Keyboard* keys = Input::get<Keyboard>();
    Vec2 delta;
    float change = 64;

    CULog("anchor %f %f", anchor.x, anchor.y);
    
//    anchor.add(0,1e-10);
//    _rootnode->setConstrained(false);
//    _rootnode->applyPan(Vec2(1,0));
    
//    _rootnode->
    
//    if(1==1)return;
    
    if (keys->keyDown(KeyCode::W)) {
        delta = Vec2(0, -change);
        if (!_isPanning) {
            _isPanning = true;
        }
        anchor.subtract(delta);

    } else if (keys->keyDown(KeyCode::A)) {
        delta = Vec2(change, 0);
        if (!_isPanning) {
            _isPanning = true;
        }
        anchor.subtract(delta);
    }
    else if (keys->keyDown(KeyCode::S)) {
        delta = Vec2(0, change);
        if (!_isPanning) {
            _isPanning = true;
        }
        anchor.subtract(delta);
    }
    else if (keys->keyDown(KeyCode::D)) {
        delta = Vec2(-change, 0);
        if (!_isPanning) {
            _isPanning = true;
        }
        anchor.subtract(delta);
    }
    else {
        _isPanning = false;
//        anchor.setZero();
    }

    if (_isPanning) {
        Vec2 transformedAnchor = anchor;
        CULog("untransformed anchor %f %f", anchor.x, anchor.y);
        transformedAnchor = _rootnode->worldToNodeCoords(transformedAnchor);
        CULog("transformed anchor %f %f", anchor.x, anchor.y);
        transformedAnchor /= _rootnode->getContentSize();
        CULog("normalized anchor %f %f", anchor.x, anchor.y);
//        _rootnode->setAnchor(transformedAnchor);
        

        gamePosition.add(delta);
        _rootnode->applyPan(delta);

        delta.setZero();
    }
}



/**
 * Handles any modifications necessary before collision resolution
 *
 * This method is called just before Box2D resolves a collision.  We use this method
 * to implement sound on contact, using the algorithms outlined in Ian Parberry's
 * "Introduction to Game Physics with Box2D".
 *
 * @param  contact  	The two bodies that collided
 * @param  oldManfold  	The collision manifold before contact
 */
void GameScene::beforeSolve(b2Contact* contact, const b2Manifold* oldManifold) {

    // TODO: Collisions with movement team.

    //float speed = 0;

    //// Use Ian Parberry's method to compute a speed threshold
    //b2Body* body1 = contact->GetFixtureA()->GetBody();
    //b2Body* body2 = contact->GetFixtureB()->GetBody();
    //b2WorldManifold worldManifold;
    //contact->GetWorldManifold(&worldManifold);
    //b2PointState state1[2], state2[2];
    //b2GetPointStates(state1, state2, oldManifold, contact->GetManifold());
    //for (int ii = 0; ii < 2; ii++) {
    //    if (state2[ii] == b2_addState) {
    //        b2Vec2 wp = worldManifold.points[0];
    //        b2Vec2 v1 = body1->GetLinearVelocityFromWorldPoint(wp);
    //        b2Vec2 v2 = body2->GetLinearVelocityFromWorldPoint(wp);
    //        b2Vec2 dv = v1 - v2;
    //        speed = b2Dot(dv, worldManifold.normal);
    //    }
    //}

    //// Play a sound if above threshold
    //if (speed > SOUND_THRESHOLD) {
    //    // These keys result in a low number of sounds.  Too many == distortion.
    //    physics2::Obstacle* data1 = reinterpret_cast<physics2::Obstacle*>(body1->GetUserData().pointer);
    //    physics2::Obstacle* data2 = reinterpret_cast<physics2::Obstacle*>(body2->GetUserData().pointer);

    //    if (data1 != nullptr && data2 != nullptr) {
    //        std::string key = (data1->getName() + data2->getName());
    //        auto source = _assets->get<Sound>(COLLISION_SOUND);
    //        if (!AudioEngine::get()->isActive(key)) {
    //            AudioEngine::get()->play(key, source, false, source->getVolume());
    //        }
    //    }
    //}

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
 * This class is the primary gameplay constroller for the demo.
 *
 * A world has its own objects, assets, and input controller.  Thus this is
 * really a mini-GameEngine in its own right.  As in 3152, we separate it out
 * so that we can have a separate mode for the loading screen.
 */
void GameScene::panScreen(const cugl::Vec2& delta) {
    if (delta.lengthSquared() == 0) {
        return;
    }

    _transform.translate(delta);
    _rootnode->chooseAlternateTransform(true);
    _rootnode->setAlternateTransform(_transform);
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

void GameScene::displayJoystick() {
    PolyFactory pf = PolyFactory();
    Vec2 center = _input.getJoystick();
    Poly2 joystickCenter = pf.makeCircle(center, _input.getJstickDeadzone());
    Poly2 joystickRadius = pf.makeCircle(center, _input.getJstickRadius());
    _jstickDeadzoneNode->setPolygon(joystickCenter);
    _jstickDeadzoneNode->setColor(Color4::RED);
    _jstickDeadzoneNode->setVisible(true);
    _jstickRadiusNode->setPolygon(joystickRadius);
    _jstickRadiusNode->setColor(Color4(1.0f, 0.0f, 0.0f, 0.3f));
    _jstickRadiusNode->setVisible(true);
}

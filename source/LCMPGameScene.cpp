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

#include "LCMPGameScene.h"
#include "LCMPGameModel.h"
#include "LCMPLevelConstants.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Regardless of logo, lock the height to this */
#define SCENE_WIDTH 1280
#define SCENE_HEIGHT  720


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
    
    // Save the network controller
    _network = network;
    
//    _game = assets->get<GameModel>(LEVEL_ONE_KEY);
    _game = GameModel::alloc(LEVEL_ONE_FILE);
    if (_game == nullptr) {
        CULog("Fail!");
        return false;
    }
    
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
    
    _quit = false;
    _isPanning = false;
    anchor.setZero();
    currPos.setZero();
    prevPos.setZero();
    gamePosition.setZero();

    _rootnode = std::dynamic_pointer_cast<scene2::ScrollPane>(_assets->get<scene2::SceneNode>("game"));

    //_rootnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _rootnode->setPosition(offset);

    _game->setAssets(_assets);
    _game->setRootNode(_rootnode);

    _rootnode->setContentSize(dimen);

    addChild(_rootnode);

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
}

void GameScene::moveScreen() { // For testing ONLY. Don't use this in game.
    Keyboard* keys = Input::get<Keyboard>();
    Vec2 delta;
    float change = 0.005;

    if (keys->keyDown(KeyCode::W)) {
        delta = Vec2(0, change);
        if (!_isPanning) {
            _isPanning = true;
            anchor = gamePosition;
            anchor.subtract(delta);
        }
        else {
            anchor.subtract(delta);
        }

    } else if (keys->keyDown(KeyCode::A)) {
        delta = Vec2(-change, 0);
        if (!_isPanning) {
            _isPanning = true;
            
            anchor.subtract(delta);
        }
        else {
            anchor.subtract(delta);
        }
    }
    else if (keys->keyDown(KeyCode::S)) {
        delta = Vec2(0, -change);
        if (!_isPanning) {
            _isPanning = true;
            anchor.subtract(delta);
        }
        else {
            anchor.subtract(delta);
        }
    }
    else if (keys->keyDown(KeyCode::D)) {
        delta = Vec2(change, 0);
        if (!_isPanning) {
            _isPanning = true;
            
            anchor.subtract(delta);
        }
        else {
            anchor.subtract(delta);
        }
    }
    else {
        _isPanning = false;
        anchor.setZero();
        delta.setZero();
    }

    if (_isPanning) {
        Vec2 transformedAnchor = anchor;
        transformedAnchor = _rootnode->worldToNodeCoords(transformedAnchor);
        transformedAnchor /= _rootnode->getContentSize();
        _rootnode->setAnchor(transformedAnchor);

        gamePosition.add(delta);
        _rootnode->applyPan(delta);

    }
}


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

    // TODO: Collisions with movement team.

    //b2Body* body1 = contact->GetFixtureA()->GetBody();
    //b2Body* body2 = contact->GetFixtureB()->GetBody();

    //// If we hit the "win" door, we are done
    //intptr_t rptr = reinterpret_cast<intptr_t>(_level->getRocket().get());
    //intptr_t dptr = reinterpret_cast<intptr_t>(_level->getExit().get());

    //if ((body1->GetUserData().pointer == rptr && body2->GetUserData().pointer == dptr) ||
    //    (body1->GetUserData().pointer == dptr && body2->GetUserData().pointer == rptr)) {
    //    setComplete(true);
    //}
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

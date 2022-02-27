//
//  LCMPGameScene.h
//  Network Lab
//
//  This class provides the main gameplay logic.
//
//  Author: Kevin Games
//  Version: 2/18/22
//

#ifndef __LCMP_GAME_SCENE_H__
#define __LCMP_GAME_SCENE_H__
#include <cugl/cugl.h>
#include <vector>
#include "LCMPNetworkController.h"
#include "LCMPInputController.h"
#include "LCMPGameModel.h"

/**
 * This class is the primary gameplay constroller for the demo.
 *
 * A world has its own objects, assets, and input controller. Thus this is
 * really a mini-GameEngine in its own right.  As in 3152, we separate it out
 * so that we can have a separate mode for the loading screen.
 */
class GameScene : public cugl::Scene2 {
protected:
//  MARK: - Properties
    
    // Controllers
    /** A reference to the Network Controller singleton instance */
    std::shared_ptr<NetworkController> _network;
    /** A reference to the Input Controller singleton instance */
    std::shared_ptr<InputController> _input;
    
    // Models
    /** A model to represent all models within the game */
    std::shared_ptr<GameModel> _game;

    /** Reference to the physics root of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _rootnode;
    /** The scale between the physics world and the screen (MUST BE UNIFORM) */
    float _scale;
    
    /** The asset manager for this game mode. */
    std::shared_ptr<cugl::AssetManager> _assets;
    /** Whether this player is the thief */
    bool _isThief;
    /** Whether this player is the host */
    bool _ishost;
    /** Whether we quit the game */
    bool _quit;


    /**
     * Activates world collision callbacks on the given physics world and sets the onBeginContact and beforeSolve callbacks
     *
     * @param world the physics world to activate world collision callbacks on
     */
    void activateWorldCollisions(const std::shared_ptr<physics2::ObstacleWorld>& world);
    

public:
//  MARK: - Constructors
    
    /**
     * Creates a new game mode with the default values.
     *
     * This constructor does not allocate any objects or start the game.
     * This allows us to use the object without a heap pointer.
     */
    GameScene() : cugl::Scene2() {}
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     *
     * This method is different from dispose() in that it ALSO shuts off any
     * static resources, like the input controller.
     */
    ~GameScene() { dispose(); }
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     */
    void dispose() override;
    
    /**
     * Initializes the controller contents, and starts the game
     *
     * In previous labs, this method "started" the scene.  But in this
     * case, we only use to initialize the scene user interface.  We
     * do not activate the user interface yet, as an active user
     * interface will still receive input EVEN WHEN IT IS HIDDEN.
     *
     * That is why we have the method {@link #setActive}.
     *
     * @param assets    The (loaded) assets for this game mode
     *
     * @return true if the controller is initialized properly, false otherwise.
     */
    bool init(const std::shared_ptr<cugl::AssetManager>& assets, std::shared_ptr<NetworkController>& network);

//  MARK: - Methods
    
    /**
     * The method called to update the scene.
     *
     * We need to update this method to constantly talk to the server
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void update(float timestep) override;
    
    /**
     * Sets whether the scene is currently active
     *
     * This method should be used to toggle all the UI elements.  Buttons
     * should be activated when it is made active and deactivated when
     * it is not.
     *
     * @param value whether the scene is currently active
     */
    virtual void setActive(bool value) override;
    
    /**
     * Returns true if the player is host.
     *
     * We may need to have gameplay specific code for host.
     *
     * @return true if the player is host.
     */
    bool isHost() const { return _ishost; }

    /**
     * Sets whether the player is host.
     *
     * We may need to have gameplay specific code for host.
     *
     * @param host  Whether the player is host.
     */
    void setHost(bool host)  { _ishost = host; _isThief = host; }
    // TODO: The host should not always be the thief

    /**
     * Returns true if the player quits the game.
     *
     * @return true if the player quits the game.
     */
    bool didQuit() const { return _quit; }
 
    /**
     * Disconnects this scene from the network controller.
     *
     * Technically, this method does not actually disconnect the network controller.
     * Since the network controller is a smart pointer, it is only fully disconnected
     * when ALL scenes have been disconnected.
     */
    void disconnect() { _network = nullptr; }

#pragma mark Collision Handling
    /**
     * Processes the start of a collision
     *
     * This method is called when we first get a collision between two objects.
     * We use this method to test if it is the "right" kind of collision.  In
     * particular, we use it to test if we make it to the win door.
     *
     * @param  contact  The two bodies that collided
     */
    void beginContact(b2Contact* contact);

    /**
     * Handles any modifications necessary before collision resolution
     *
     * This method is called just before Box2D resolves a collision.  We use
     * this method to implement sound on contact, using the algorithms outlined
     * in Ian Parberry's "Introduction to Game Physics with Box2D".
     *
     * @param  contact  The two bodies that collided
     * @param  contact  The collision manifold before contact
     */
    void beforeSolve(b2Contact* contact, const b2Manifold* oldManifold);

};

#endif /* __NL_GAME_SCENE_H__ */

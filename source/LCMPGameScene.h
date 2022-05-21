//
//  LCMPGameScene.h
//  Low Control Mall Patrol
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
#include "LCMPAudioController.h"
#include "LCMPCollisionController.h"
#include "LCMPInputController.h"
#include "LCMPUIController.h"
#include "LCMPGameModel.h"

/**
 * This class is the primary gameplay controller for the demo.
 *
 * A world has its own objects, assets, and input controller. Thus this is
 * really a mini-GameEngine in its own right.  As in 3152, we separate it out
 * so that we can have a separate mode for the loading screen.
 */
class GameScene : public cugl::Scene2 {
//  MARK: - Enumerations
public:
    /** The different states to represent logic in GameScene as a Finite State Machine */
    enum State {
        /** The state when the game has not begun, and the game is being initialized */
        INIT,
        /** The state when the game is counting down to start */
        COUNTDOWN,
        /** The state when main gameplay occurs */
        GAME,
        /** The state where the game is over and we wait for reseting */
        DONE,
        /** The state where the gae has the settings menu open */
        SETTINGS,
        
        SHOP
    };
    
protected:
//  MARK: - Properties
    
    // Controllers
    /** A reference to the Asset Manager */
    std::shared_ptr<cugl::scene2::ActionManager> _actions;
    /** A reference to the Network Controller instance */
    std::shared_ptr<NetworkController> _network;
    /** A reference to the Audio Controller instance */
    std::shared_ptr<AudioController> _audio;
    /** The Collision Controller instance */
    CollisionController _collision;
    /** The Input Controller instance */
    InputController _input;
    /** The UI Controller instance */
    UIController _ui;
    
    // Models
    /** A model to represent all models within the game */
    std::shared_ptr<GameModel> _game;
    
    // Views
    /** Reference to the ceiling node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _ceilingnode;
    /** Reference to the floor node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _floornode;
    /** Reference to the background node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _backgroundnode;
    /** Reference to the physics node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _worldnode;
    /** Reference to the debug node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _debugnode;
    /** Reference to the ui node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _uinode;

    // Pointers
    /** The Box2D world */
    std::shared_ptr<cugl::physics2::ObstacleWorld> _world;
    /** The asset manager for this game mode */
    std::shared_ptr<cugl::AssetManager> _assets;
    /** Font for Scene Nodes */
    shared_ptr<cugl::Font> _font;
    
    // Scaling
    /** The actual size of the display. */
    cugl::Size _screenSize;
    /** The locked size of the display. */
    cugl::Size _dimen;
    /** The amount to move the world node by to center it in the scene */
    cugl::Vec2 _offset;
    /** The scale between the physics world and the screen (SCREEN UNITS / BOX2D WORLD UNITS) */
    float _scale;
    
    // Timing
    /** The time for the state GAME */
    float _gameTime;
    /** The time for the state DONE */
    float _doneTime;
    /** The last time the cop tackled */
    float _tackleTime;
    
    // Control
    /** The current state of the game */
    State _state;
    /** The unique player number of this player */
    int _playerNumber;
    /** Whether this player is the thief */
    bool _isThief;
    /** Whether this player is the host */
    bool _isHost;
    /** Whether we quit the game */
    bool _quit;
    /** Whether the thief wins or loses */
    bool _isThiefWin;
    
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
    bool init(const std::shared_ptr<cugl::AssetManager>& assets,
              std::shared_ptr<NetworkController>& network,
              std::shared_ptr<AudioController>& audio,
              std::shared_ptr<cugl::scene2::ActionManager>& actions);

//  MARK: - Methods
    
    bool isThief() { return _isThief; }
    
    bool isThiefWin() { return _isThiefWin; }
    
    GameScene::State getState() { return _state; }

    /**
     * Sets whether the player is host.
     *
     * We may need to have gameplay specific code for host.
     *
     * @param host  Whether the player is host.
     */
    void start(bool host);
    
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
    
    /**
     * Resets the status of the game so that we can play again.
    */
   void reset() override;

private:
//  MARK: - States
    
    /**
     * The update method for when we are in state INIT
     */
    void stateInit(float timestep);
    
    /**
     * The update method for when we are in state GAME
     */
    void stateGame(float timestep);
    
    /**
     * The update method for when we are in state DONE
     */
    void stateDone(float timestep);

    /**
     * The update method for when we are in state SETTINGS
     */
    void stateSettings(float timestep);
    
//  MARK: - Helpers
    
    /**
     * Updates local players (own player and non-playing players)
     */
    void updateLocal(float timestep, cugl::Vec2 movement, bool dtap,
                     float swipe, cugl::Vec2 tackle);
    
    /**
     * Updates and networks the thief and any actions it can perform
     */
    void updateThief(float timestep, cugl::Vec2 movement, bool activate);
    
    /**
     * Updates and networks a cop and any actions it can perform
     */
    void updateCop(float timestep, int copID, cugl::Vec2 movement, bool swipe, cugl::Vec2 tackle, bool dtap);
    
    /**
     * Updates based on data received over the network
     */
    void updateNetwork(float timestep);
    
    /**
     * Sets camera position at the start of the game.
     */
    void initCamera();

    /**
     * Updates camera based on the position of the controlled player
     */
    void updateCamera(float timestep);
    
    /**
     * Updates the floor based on the camera
     */
    void updateFloor(float timestep);
    
    /**
     * Updates the UI and repositions the UI Node
     */
    void updateUI(float timestep, bool isThief, cugl::Vec2 movement,
                  bool didPress, cugl::Vec2 origin, cugl::Vec2 position, int playerNumber);
    
    /**
     * Updates the ordering of the nodes in the World Node
     */
    void updateOrder(float timestep);
    
};

#endif /* __NL_GAME_SCENE_H__ */

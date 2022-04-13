//
//  LCMPCustomizeScene.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//

#ifndef LCMPCustomizeScene_h
#define LCMPCustomizeScene_h
#include <cugl/cugl.h>
#include <vector>
#include "LCMPNetworkController.h"
#include "LCMPAudioController.h"
#include "LCMPInputController.h"
#include <cugl/scene2/actions/CUActionManager.h>
#include <cugl/scene2/actions/CUAnimateAction.h>
#include <cugl/scene2/actions/CUMoveAction.h>

class CustomizeScene : public cugl::Scene2 {
public:
//  MARK: - Enumerations
    
    /** The configuration status */
    enum Status {
        /** Host is waiting on a connection */
        IDLE,
        /** Host is waiting on all players to join */
        WAIT,
        /** Time to start the game */
        START,
        /** Game was aborted; back to main menu */
        ABORT
    };
    
protected:
//  MARK: - Properties
    
    /** The Input Controller instance */
    InputController _input;
    
    /** A reference to the Network Controller instance */
    std::shared_ptr<NetworkController> _network;
    
    /** The asset manager for this scene. */
    std::shared_ptr<cugl::AssetManager> _assets;
    
    /** A reference to the Action Manager */
    std::shared_ptr<cugl::scene2::ActionManager> _actions;
    
    /** The sound controller for the game */
    std::shared_ptr<AudioController> _audio;
    
    /** The top-level node for displaying the player */
//    std::shared_ptr<cugl::scene2::SceneNode> _node;
    
    /** The back button for the menu scene */
    std::shared_ptr<cugl::scene2::Button> _backout;
    
    /** The menu button for starting a game */
    std::shared_ptr<cugl::scene2::Button> _startgame;
    
    /** The nodes for the player skins */
    std::vector<std::shared_ptr<cugl::Texture>> _spriteSheets;
    std::vector<std::shared_ptr<cugl::scene2::SpriteNode>> _spriteNodes;
    std::vector<std::shared_ptr<cugl::scene2::Animate>> _animations;
    std::vector<string> _keys;
    
    /** The movement actions */
    std::shared_ptr<cugl::scene2::MoveBy> _moveLeft;
    std::shared_ptr<cugl::scene2::MoveBy> _moveRight;
    
    cugl::Size _dimen;

    bool _didLeft;
    bool _isThief;
    float _customTime;
    float _lastChoice;
    
    /** Whether or not the game is being sent to the customize menu. */
    bool _didStart;
    
    /** The current status */
    Status _status;
    
public:
    /** Which texture has been chosen */
    int skin;
    string skinKey;
    
//  MARK: - Constructors
    
    /**
     * Creates a new host scene with the default values.
     *
     * This constructor does not allocate any objects or start the game.
     * This allows us to use the object without a heap pointer.
     */
    CustomizeScene() : cugl::Scene2() {}
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     *
     * This method is different from dispose() in that it ALSO shuts off any
     * static resources, like the input controller.
     */
    ~CustomizeScene() { dispose(); }
    
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
    
    /**
     * The method called to update the scene.
     *
     * We need to update this method to constantly talk to the server
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void update(float timestep) override;
    
    /** Updates scene based on player input */
    void updateInput(float timestep);
    
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
     * Returns the scene status.
     *
     * Any value other than WAIT will transition to a new scene.
     *
     * @return the scene status
     *
     */
    Status getStatus() const { return _status; }
    
    /**
     * Disconnects this scene from the network controller.
     *
     * Technically, this method does not actually disconnect the network controller.
     * Since the network controller is a smart pointer, it is only fully disconnected
     * when ALL scenes have been disconnected.
     */
    void disconnect() { _network = nullptr; }
    
    /**
     * Updates the text in the given button.
     *
     * Techincally a button does not contain text. A button is simply a scene graph
     * node with one child for the up state and another for the down state. So to
     * change the text in one of our buttons, we have to descend the scene graph.
     * This method simplifies this process for you.
     *
     * @param button    The button to modify
     * @param text      The new text value
     */
    void updateText(const std::shared_ptr<cugl::scene2::Button>& button, const std::string text);
    
    /**
     * Reconfigures the start button for this scene
     *
     * This is necessary because what the buttons do depends on the state of the
     * networking.
     */
    void configureStartButton();
    
    /**
     * Connects to the game server as specified in the assets file
     *
     * The {@link #init} method set the configuration data. This method simply uses
     * this to create a new {@Link NetworkConnection}. It also immediately calls
     * {@link #checkConnection} to determine the scene state.
     *
     * @return true if the connection was successful
     */
    bool connect();

    /**
     * Checks that the network connection is still active.
     *
     * Even if you are not sending messages all that often, you need to be calling
     * this method regularly. This method is used to determine the current state
     * of the scene.
     *
     * @return true if the network connection is still active.
     */
    bool checkConnection();
    
    /**
     * Starts the game.
     *
     * This method is called once the requisite number of players have connected.
     * It locks down the room and sends a "start game" message to all other
     * players.
     */
    void startGame();
    
    /** Displays the skins */
    void displaySkins();
};

#endif /* LCMPCustomizeScene_h */

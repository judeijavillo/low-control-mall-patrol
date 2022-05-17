//
//  LCMPVictoryScene.h
//  Low Control Mall Patrol
//
//  This class provides the win condition logic.
//
//  Author: Kevin Games
//  Version: 4/23/22
//

#ifndef LCMPVictoryScene_h
#define LCMPVictoryScene_h

#include <cugl/cugl.h>
#include "LCMPNetworkController.h"
#include "LCMPAudioController.h"
#include "LCMPInputController.h"
#include <cugl/scene2/actions/CUActionManager.h>
#include <cugl/scene2/actions/CUAnimateAction.h>
#include <cugl/scene2/actions/CUMoveAction.h>

class VictoryScene : public cugl::Scene2 {
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
    
    /** A reference to the Network Controller instance */
    std::shared_ptr<NetworkController> _network;
    
    /** The asset manager for this scene. */
    std::shared_ptr<cugl::AssetManager> _assets;
    
    /** A reference to the Action Manager */
    std::shared_ptr<cugl::scene2::ActionManager> _actions;
    
    /** The sound controller for the game */
    std::shared_ptr<AudioController> _audio;
    
    /** The replay button for the victory scene */
    std::shared_ptr<cugl::scene2::Button> _replayButton;
    /** The leave button for the victory scene */
    std::shared_ptr<cugl::scene2::Button> _leaveButton;
    /** The victory text */
    std::shared_ptr<cugl::scene2::Label> _text;
    std::shared_ptr<cugl::scene2::PolygonNode> _thiefBanner;
    std::shared_ptr<cugl::scene2::PolygonNode> _copBanner;
    
    /** The amount to move the world node by to center it in the scene */
    cugl::Vec2 _offset;

    /** The current status */
    Status _status;
    
    /** Whether the player is the thief or not */
    bool _isThief;
    /** Whether the thief won or not */
    bool _thiefWin;

public:
    /**
     * Creates a new host scene with the default values.
     *
     * This constructor does not allocate any objects or start the game.
     * This allows us to use the object without a heap pointer.
     */
    VictoryScene() : cugl::Scene2() {}
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     *
     * This method is different from dispose() in that it ALSO shuts off any
     * static resources, like the input controller.
     */
    ~VictoryScene() { dispose(); }
    
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
              std::shared_ptr<cugl::scene2::ActionManager>& actions,
              bool thiefWin);
    
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
    virtual void setActive(bool value, bool isThief, bool thiefWin);
    
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
    
    void updateMessage();

};
#endif /* LCMPVictoryScene_h */

//
//  LCMPClientScene.h
//  Low Control Mall Patrol
//
//  This class represents the scene for the client when joining a game. Normally
//  this class would be combined with the class for the client scene (as both
//  initialize the network controller).  But we have separated to make the code
//  a little clearer for this lab.
//
//  Originator: Walker White, Aidan Hobler
//  Author: Kevin Games
//  Version: 2/18/22
//

#ifndef __LCMP_CLIENT_SCENE_H__
#define __LCMP_CLIENT_SCENE_H__
#include <cugl/cugl.h>
#include <vector>
#include "LCMPNetworkController.h"
#include "LCMPAudioController.h"

/**
 * This class provides the interface to join an existing game.
 *
 * Most games have a since "matching" scene whose purpose is to initialize the
 * network controller.  We have separate the host from the client to make the
 * code a little more clear.
 */
class ClientScene : public cugl::Scene2 {
public:
//  MARK: - Enumerations
    
    /** The configuration status */
    enum Status {
        /** Client has not yet entered a room */
        IDLE,
        /** Client is connecting to the host */
        JOIN,
        /** Client is waiting on host to start game */
        WAIT,
        /** Time to start the game */
        START,
        /** Game was aborted; back to main menu */
        ABORT
    };
    
protected:
//  MARK: - Properties
    
    /** The amount to move the world node by to center it in the scene */
    cugl::Vec2 _offset;
    
    /** A reference to the Network Controller singleton instance */
    std::shared_ptr<NetworkController> _network;
    
    /** The asset manager for this scene. */
    std::shared_ptr<cugl::AssetManager> _assets;
    
    /** The sound controller for the game */
    std::shared_ptr<AudioController> _audio;

    /** The keypad buttons */
    unordered_set<std::shared_ptr<cugl::scene2::Button>> _keypadButtons;
    /** The scene node containing the keypad */
    std::shared_ptr<cugl::scene2::SceneNode> _keypad;
    /** The scene node containing the donut box front */
    std::shared_ptr<cugl::scene2::PolygonNode> _donutFront;
    /** The menu button for changing a player's gender */
    std::shared_ptr<cugl::scene2::Button> _genderButton;
    /** The menu button for starting a game */
    std::shared_ptr<cugl::scene2::Button> _startgame;
    /** The back button for the menu scene */
    std::shared_ptr<cugl::scene2::Button> _backout;
    /** The game id label (for updating) */
    std::shared_ptr<cugl::scene2::Label> _gameid;
    /** The players label (for updating) */
    std::shared_ptr<cugl::scene2::Label> _info;
    
    /** A mapping from player number to character node */
    std::vector<std::shared_ptr<cugl::scene2::SpriteNode>> _nodes;
    /** The sprite of the thief */
    std::shared_ptr<cugl::scene2::SpriteNode> _thiefNode;
    /** The sprite of cop 1 */
    std::shared_ptr<cugl::scene2::SpriteNode> _cop1Node;
    /** The sprite of cop 2 */
    std::shared_ptr<cugl::scene2::SpriteNode> _cop2Node;
    /** The sprite of cop 3 */
    std::shared_ptr<cugl::scene2::SpriteNode> _cop3Node;
    /** The sprite of cop 4 */
    std::shared_ptr<cugl::scene2::SpriteNode> _cop4Node;
    /** The cop buttons */
    std::vector<std::shared_ptr<cugl::scene2::Button>> _copButtons;
    
    /** A mapping from player number to player username textfield */
    std::vector<std::shared_ptr<cugl::scene2::TextField>> _players;
    /** The player 1 label (for updating) */
    std::shared_ptr<cugl::scene2::TextField> _player1;
    /** The player 2 label (for updating) */
    std::shared_ptr<cugl::scene2::TextField> _player2;
    /** The player 3 label (for updating) */
    std::shared_ptr<cugl::scene2::TextField> _player3;
    /** The player 4 label (for updating) */
    std::shared_ptr<cugl::scene2::TextField> _player4;
    /** The player 5 label (for updating) */
    std::shared_ptr<cugl::scene2::TextField> _player5;
    
    /** The keys to access player skins */
    std::vector<string> _skinKeys;
    /** The references to player skins */
    std::vector<std::shared_ptr<cugl::scene2::PolygonNode>> _skins;
    
    bool _sixteenNineAspectRatio;
    
    /** The current animation frame */
    int _aniFrame;
    /** The previous timestep. */
    float _prevTime;
    
    /** The current status */
    Status _status;

public:
    /** The skin the player chooses */
    int skinChoice;
//  MARK: - Constructors
    /**
     * Creates a new client scene with the default values.
     *
     * This constructor does not allocate any objects or start the game.
     * This allows us to use the object without a heap pointer.
     */
    ClientScene() : cugl::Scene2() {}
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     *
     * This method is different from dispose() in that it ALSO shuts off any
     * static resources, like the input controller.
     */
    ~ClientScene() { dispose(); }
    
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
              bool sixteenNineAspectRatio);
    
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

private:
//  MARK: - Helpers
    
    /**
     * Connects to the game server as specified in the assets file
     *
     * The {@link #init} method set the configuration data. This method simply uses
     * this to create a new {@Link NetworkConnection}. It also immediately calls
     * {@link #checkConnection} to determine the scene state.
     *
     * @param room  The room ID to use
     *
     * @return true if the connection was successful
     */
    bool connect(const std::string room);
    
    /**
     * Helper for showing and hiding the lobby
     */
    void showLobby(bool lobby);
    
    /**
     * Plays animations for the players and sets their names
     */
    void updateLobby(float timestep);
    
    /**
     * Updates the player customizations
     */
    void updateSkins();

//  MARK: - Callbacks
    
    /**
     * Helper for keypad button callback
     */
    void pressButton(const std::string& name, bool down, int buttonID);
    
};

#endif /* __LCMP_GAME_SCENE_H__ */

//
//  NetworkController.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/19/22
//

#ifndef __LCMP_NETWORK_CONTROLLER_H__
#define __LCMP_NETWORK_CONTROLLER_H__
#include <cugl/cugl.h>
#include "LCMPGameModel.h"

/**
 * The controller that handles all communication between clients and the server
 */
class NetworkController {
public:
//  MARK: - Enumerations
    
    /** The different statuses of the Network Controller  */
    enum Status {
        /** Not connected to server */
        IDLE,
        /** Connecting to server, waiting for roomID (host) or playerID (client) */
        CONNECTING,
        /** Connected, waiting for all players to join */
        WAIT,
        /** Time to start the game */
        START,
        /** The game was abortted  */
        ABORT
    };
    
    /** The different signals that the Network Controller can send */
    enum Signal {
        /** It's time to start the game */
        START_GAME = 0,
        /** What follows is which cop to update, its x and y position, and its x and y velocity */
        COP_MOVEMENT = 1,
        /** What follows is the thief's x and y position, and its x and y velocity */
        THIEF_MOVEMENT = 2,
        /** What follows is which trap to activate */
        TRAP_ACTIVATION = 3,
        /** This indicates that the game is over (cops won) */
        GAME_OVER = 4
    };

protected:
//  MARK: - Properties
    
    /** The serializer for sending byte vectors */
    cugl::NetworkSerializer _serializer;
    /** The deserializer for receiving byte vectors */
    cugl::NetworkDeserializer _deserializer;
    /** The configuration settings for establishing the network connection */
    cugl::NetworkConnection::ConnectionConfig _config;
    /** The network connection (as made by this scene) */
    std::shared_ptr<cugl::NetworkConnection> _connection;
    
    /** Whether the connection being made is for a host or not */
    bool _isHost;
    /** Whether the host decides this player is the thief or not */
    int _playerNumber;
    
    /** The current status of the Network Controller */
    Status _status;
    
public:
//  MARK: - Constructors
    
    /**
     * The constructor for a Network Controller
     */
    NetworkController();
    
    /**
     * The destructor for a Newtork Controller
     */
    ~NetworkController() { dispose(); };
    
    /**
     * Releases all resources for this singleton Network Controller
     */
    void dispose();
    
//  MARK: - Methods
    
    /**
     * Sets whether or not the connection should be made for a host
     */
    void setHost(bool value) { _isHost = value; }
    
    /**
     * Returns true iff the Network Controller is connected
     */
    bool isConnected() { return _connection != nullptr; }
    
    /**
     * Returns the player number
     */
    int getPlayerNumber() const { return _playerNumber; }
    
    /**
     * Returns the player ID or empty.
     */
    std::optional<uint8_t> getPlayerID() const { return _connection->getPlayerID(); }

    /**
     * Returns the room ID or empty string
     */
    std::string getRoomID() const { return _connection->getRoomID(); }

    /**
     * Returns the number of players currently connected to this game
     */
    uint8_t getNumPlayers() const { return _connection->getNumPlayers(); }
    
    /**
     * Returns the status of the Network Controller
     */
    Status getStatus() { return _status; }
    
    /**
     * Establishes a host connection with the server
     */
    bool connect();
    
    /**
     * Establishes a client connection with the server
     */
    bool connect(const std::string room);
    
    /**
     * Severs the connection with the server
     */
    void disconnect() { _connection = nullptr; }
    
//  MARK: - Matchmaking
    
    /**
     * Checks the connection and updates the status accordingly (pre-game)
     */
    void update();
    
    /**
     * Sends a message intended for the host a unique player 
     */
    
    /**
     * Sends a byte vector to start the game
     */
    void sendStartGame();

    
//  MARK: - Gameplay

    /**
     * Checks the connection, updates the status accordingly, and updates the game (during game)
     */
    void update(std::shared_ptr<GameModel>& game);
    /**
     * Sends a byte vector to update thief movement
     */
    void sendThiefMovement(std::shared_ptr<GameModel>& game, cugl::Vec2 force);
    
    /**
     * Sends a byte vector to update thief movement
     */
    void sendCopMovement(std::shared_ptr<GameModel>& game, cugl::Vec2 force, int copID);
    
    /**
     * Sends a byte vector to activate a trap
     */
    void sendTrapActivation(int trapID);
    
    /**
     * Sends a byte vector to indicate game over
     */
    void sendGameOver();
    
};

#endif /* __LCMP_NETWORK_CONTROLLER_H__ */

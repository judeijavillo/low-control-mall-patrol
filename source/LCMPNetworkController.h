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

protected:
//  MARK: - Properties
    
    /** The configuration settings for establishing the network connection */
    cugl::NetworkConnection::ConnectionConfig _config;
    /** The network connection (as made by this scene) */
    std::shared_ptr<cugl::NetworkConnection> _connection;
    
    /** Whether the connection being made is for a host or not */
    bool _isHost;
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
     * Returns the player ID or empty.
     */
    std::optional<uint8_t> getPlayerID() const { return _connection->getPlayerID(); }

    /**
     * Returns the room ID or empty string.
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
    
    /**
     * Checks the connection and updates the status accordingly (pre-game)
     */
    void update();
    
    /**
     * Checks the connection, updates the status accordingly, and updates the game (during game)
     */
    void update(std::shared_ptr<GameModel> game);
    
    /**
     * Sends a byte vector over the Network Connection
     */
    void send(const std::vector<uint8_t>& data) { _connection->send(data); }
    
};

#endif /* __LCMP_NETWORK_CONTROLLER_H__ */

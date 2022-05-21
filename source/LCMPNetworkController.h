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
#include <slikenet/TCPInterface.h>
#include <slikenet/HTTPConnection2.h>
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
        ABORT,
        /** The host has requested a rematch */
        REMATCH
    };
    
    /** The different signals that the Network Controller can send */
    enum Signal {
        /** A basic ping */
        PING,
        /** State of the lobby */
        PLAYER,
        /** It's time to start the game */
        START_GAME,
        /** What follows is which cop to update, its x and y position, and its x and y velocity */
        COP_MOVEMENT,
        /** What follows is the thief's x and y position, and its x and y velocity */
        THIEF_MOVEMENT,
        /** What follows is which trap to activate */
        TRAP_ACTIVATION,
        /** This indicates that the game is over */
        GAME_OVER,
        /** This indicates that the host wants a rematch */
        PLAY_AGAIN
    };

//  MARK: - Structs
    
    /** A data representation of a Player */
    struct Player {
        bool male;
        int playerID;
        int playerNumber;
        float lastPing;
        string username;
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
    /** A reference to the HTTP Connection */
    SLNet::HTTPConnection2* _http;
    /** A reference to the TCP Interface */
    SLNet::TCPInterface* _tcp;
    
    /** The mapping from player ID to Player struct */
    std::unordered_map<int, Player> _players;
    /** The level for the client to read what map to load */
    std::string _level;
    /** The current status of the Network Controller */
    Status _status;
    /** Whether the connection being made is for a host or not */
    bool _isHost;
    /** Whether the host decides this player is the thief or not */
    int _playerNumber;
    
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
    
    /**
     * Initializes a Network Controller
     */
    bool init();
    
//  MARK: - Methods
    
    /**
     * Returns whether or not the player is a host
     */
    bool isHost() { return _isHost; }
    
    /**
     * Sets whether or not the connection should be made for a host
     */
    void setHost(bool value) { _isHost = value; }
    
    /**
     * Returns true iff the Network Controller is connected
     */
    bool isConnected() { return _connection != nullptr; }
    
    /**
     * Returns whether or not a player is still active
     */
    bool isPlayerConnected(int playerId) { return _connection->isPlayerActive(playerId); }
    
    /**
     * Returns the player associated with this player ID
     */
    Player getPlayer(int playerID) { return _players[playerID]; }
    
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
     * Returns the level for the client
     */
    std::string getLevel() { return _level; }
    
    /**
     * Returns the mapping of playerID to male boolean
     */
    std::unordered_map<int, bool> getMales();
    
    
    /**
     * Sets the status of the Network Controller
     */
     void setStatus(Status value) { _status = value; }
    
    /**
     * Sets this player's username
     */
    void setUsername(std::string username) { _players[*getPlayerID()].username = username; }
    
    /**
     * Sets this player's gender
     */
    void toggleGender() { _players[*getPlayerID()].male = !_players[*getPlayerID()].male; }
    
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
    void update(float timestep);
    
    /**
     * Sends a ping to verify that the player is still active
     */
    void sendPing();
    
    /**
     * Sends the state of a given player
     */
    void sendPlayer();
    
    /**
     * Sends a byte vector to start the game
     */
    void sendStartGame(std::string level, bool randomThief = false, int thiefChoice = 0);

    
//  MARK: - Gameplay

    /**
     * Checks the connection, updates the status accordingly, and updates the game (during game)
     */
    void update(float timestep, std::shared_ptr<GameModel>& game);
    
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
    void sendGameOver(bool thiefWin);
    
    /**
     * Sends a byte vector to indicate to rematch
     */
    void sendRematch();
        
//  MARK: - Server
    
    /**
     * Makes a request for a test endpoint
     */
    void getTest();
    
    /**
     * Makes a request to check if the suggested roomID has been assigned a roomID
     */
    void getRoom(std::string roomID);
    
    /**
     * Makes a request to post a public room
     */
    void postRoom(std::string roomID);
    
    /**
     * Makes a request to delete a public room
     */
    void deleteRoom(std::string roomID);
    
    /**
     * Returns the content body of the response of a previously made request as a JsonValue
     */
    std::shared_ptr<cugl::JsonValue> readResponse();
    
private:
//  MARK: - Helpers
    
    /**
     * Makes an HTTP request using the given RakString
     */
    void makeRequest(SLNet::RakString request);
    
    /**
     * Makes a GET request at a given endpoint
     */
    void makeGETrequest(std::string endpoint);
    
    /**
     * Makes a POST request at a given endpoint
     */
    void makePOSTrequest(std::string endpoint, std::string body);
    
    /**
     * Makes a PATCH request at a given endpoint
     */
    void makePATCHrequest(std::string endpoint, std::string body);
    
    /**
     * Makes a DELETE request at a given endpoint
     */
    void makeDELETErequest(std::string endpoint);
    
};

#endif /* __LCMP_NETWORK_CONTROLLER_H__ */

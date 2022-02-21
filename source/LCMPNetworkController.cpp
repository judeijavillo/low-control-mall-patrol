//
//  LCMPNetworkController.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/19/22
//

#include "LCMPNetworkController.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** The address of the server */
#define SERVER_ADDRESS  "3.145.38.224"
/** The port of the server */
#define SERVER_PORT     61111
/** The maximum number of players allowed in a single room */
#define SERVER_MAX      5
/** The version of the server */
#define SERVER_VERSION  0

//  MARK: - Constructors

/**
 * Initializes a Network Controller. Returns true iff successful.
 */
NetworkController::NetworkController() {
    _config.punchthroughServerAddr = SERVER_ADDRESS;
    _config.punchthroughServerPort = SERVER_PORT;
    _config.maxNumPlayers = SERVER_MAX;
    _config.apiVersion = SERVER_VERSION;
    _status = IDLE;
}

/**
 * Releases all resources for this Network Controller
 */
void NetworkController::dispose() {
    _connection = nullptr;
}

//  MARK: - Methods

/**
 * Establishes a host connection with the server
 */
bool NetworkController::connect() {
    _connection = NetworkConnection::alloc(_config);
    update();
    return _status != IDLE;
}

/**
 * Establishes a client connection with the server
 */
bool NetworkController::connect(const std::string room) {
    _connection = NetworkConnection::alloc(_config, room);
    update();
    return _status != IDLE;
}

/**
 * Checks the connection and updates the status accordingly (pre-game)
 */
void NetworkController::update() {
    switch (_connection->getStatus()) {
    case NetworkConnection::NetStatus::Pending:
        _status = CONNECTING;
        break;
    case NetworkConnection::NetStatus::Connected:
        _status = WAIT;
        break;
    case NetworkConnection::NetStatus::Disconnected:
    case NetworkConnection::NetStatus::Reconnecting:
    case NetworkConnection::NetStatus::RoomNotFound:
    case NetworkConnection::NetStatus::ApiMismatch:
    case NetworkConnection::NetStatus::GenericError:
        _status = IDLE;
        break;
    }
    
    _connection->receive([this](const std::vector<uint8_t> data) {
        // TODO: Add more functionality for getting client info, like names
        if (_isHost) return;
        _status = (data.at(0) == 255) ? START : _status;
        
    });
}

/**
 * Checks the connection, updates the status accordingly, and updates the game (during game)
 */
void NetworkController::update(std::shared_ptr<GameModel> game) {
    // TODO: Implement network updates
}

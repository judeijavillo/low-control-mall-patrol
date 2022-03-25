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

//  MARK: - Matchmaking

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
    
    _connection->receive([this](const std::vector<uint8_t> msg) {
        // TODO: Add more functionality for getting client info, like names
        if (_isHost) return;
        _deserializer.receive(msg);
        vector<float> data = _deserializer.readFloatVector();
        _deserializer.reset();
        
        switch ((int) data.at(0)) {
        case START_GAME:
            _status = START;
            int i = 2;
            while (i < data.size()) {
                int player = (int) data.at(i);
                int role = (int) data.at(i+1);
                if (player == this->getPlayerID()) _playerNumber = role;
                i += 2;
            }
            break;
        }
    });
}

/**
 * Sends a byte vector to start the game
 */
void NetworkController::sendStartGame(bool randomThief, int thiefChoice) {
    vector<float> data;
    data.push_back(START_GAME);
    data.push_back(getNumPlayers());
    
    int count = 0;
    int thief = randomThief ? rand() % getNumPlayers() : thiefChoice;
    for (int playerID = 0; playerID < 5; playerID++) {
        _players[playerID] = {playerID, playerID == getPlayerID() ? -1 : count};
        data.push_back(playerID);
        if (thief == playerID) {
            if (playerID == getPlayerID()) _playerNumber = -1;
            data.push_back(-1);
        } else {
            if (playerID == getPlayerID()) _playerNumber = count;
            data.push_back(count);
            count++;
        }
    }
    _serializer.writeFloatVector(data);
    _connection->send(_serializer.serialize());
    _serializer.reset();
}

//  MARK: - Gameplay

/**
 * Checks the connection, updates the status accordingly, and updates the game (during game)
 */
void NetworkController::update(std::shared_ptr<GameModel>& game) {
    // Give up if connection is not established
    if (_connection == nullptr) return;
    
    // Handle different
    if (_connection->getStatus() != NetworkConnection::NetStatus::Connected) {
        // TODO: Handle other network statuses
        return;
    }
    
    _connection->receive([this, game](const std::vector<uint8_t> msg) {
        _deserializer.receive(msg);
        vector<float> data = _deserializer.readFloatVector();
        _deserializer.reset();
        
        switch ((int) data.at(0)) {
        case COP_MOVEMENT:
            game->updateCop(Vec2(data.at(1), data.at(2)),
                            Vec2(data.at(3), data.at(4)),
                            Vec2(data.at(5), data.at(6)),
                            Vec2(data.at(7), data.at(8)),
                            Vec2(data.at(9), data.at(10)),
                            (float) data.at(11),
                            (bool) data.at(12),
                            (bool) data.at(13),
                            (bool) data.at(14),
                            (int) data.at(15));
            break;
        case THIEF_MOVEMENT:
            game->updateThief(Vec2(data.at(1), data.at(2)),
                              Vec2(data.at(3), data.at(4)),
                              Vec2(data.at(5), data.at(6)));
            break;
        case TRAP_ACTIVATION:
            game->activateTrap((int) data.at(1));
            break;
        case GAME_OVER:
            game->setGameOver(true);
            break;
        default:
            break;
        }
    });
}

/**
 * Sends a byte vector to update thief movement
 */
void NetworkController::sendThiefMovement(std::shared_ptr<GameModel>& game, Vec2 force) {
    if (_connection == nullptr) return;
    std::shared_ptr<ThiefModel> thief = game->getThief();
    Vec2 position = thief->getPosition();
    Vec2 velocity = thief->getVelocity();
    vector<float> data;
    
    data.push_back(THIEF_MOVEMENT);
    data.push_back(position.x);
    data.push_back(position.y);
    data.push_back(velocity.x);
    data.push_back(velocity.y);
    data.push_back(force.x);
    data.push_back(force.y);
    
    _serializer.writeFloatVector(data);
    _connection->send(_serializer.serialize());
    _serializer.reset();
}

/**
 * Sends a byte vector to update cop movement
 */
void NetworkController::sendCopMovement(std::shared_ptr<GameModel>& game, Vec2 force, int copID) {
    if (_connection == nullptr) return;
    std::shared_ptr<CopModel> cop = game->getCop(copID);
    Vec2 position = cop->getPosition();
    Vec2 velocity = cop->getVelocity();
    Vec2 tackleDirection = cop->getTackleDirection();
    Vec2 tacklePosition = cop->getTacklePosition();
    float tackleTime = cop->getTackleTime();
    bool tackling = cop->getTackling();
    bool caughtThief = cop->getCaughtThief();
    bool tackleSucessful = cop->getTackleSuccessful();
    
    vector<float> data;
    
    data.push_back(COP_MOVEMENT);
    data.push_back(position.x);
    data.push_back(position.y);
    data.push_back(velocity.x);
    data.push_back(velocity.y);
    data.push_back(force.x);
    data.push_back(force.y);
    data.push_back(tackleDirection.x);
    data.push_back(tackleDirection.y);
    data.push_back(tacklePosition.x);
    data.push_back(tacklePosition.y);
    data.push_back(tackleTime);
    data.push_back(tackling);
    data.push_back(caughtThief);
    data.push_back(tackleSucessful);
    data.push_back(copID);
    
    _serializer.writeFloatVector(data);
    _connection->send(_serializer.serialize());
    _serializer.reset();
    
}

/**
 * Sends a byte vector to activate a trap
 */
void NetworkController::sendTrapActivation(int trapID) {
    if (_connection == nullptr) return;
    vector<float> data;
    data.push_back(TRAP_ACTIVATION);
    data.push_back(trapID);
    
    _serializer.writeFloatVector(data);
    _connection->send(_serializer.serialize());
    _serializer.reset();
    
}

/**
 * Sends a byte vector to indicate game over
 */
void NetworkController::sendGameOver() {
    if (_connection == nullptr) return;
    vector<float> data;
    data.push_back(GAME_OVER);
    
    _serializer.writeFloatVector(data);
    _connection->send(_serializer.serialize());
    _serializer.reset();
}

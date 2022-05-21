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

// Webserver Constants
/** The address of the webserver */
#define WEBSERVER_ADDRESS   "3.15.40.206"
/** The port of the webserver */
#define WEBSERVER_PORT      8080

// NAT Punchthrough server Constants
/** The address of the server */
#define SERVER_ADDRESS  "3.15.40.206"
/** The port of the server */
#define SERVER_PORT     61111
/** The maximum number of players allowed in a single room */
#define SERVER_MAX      5
/** The version of the server */
#define SERVER_VERSION  0

/** The number of messages the network controller can receive at a given update call */
#define NETWORK_STEP    10

//  MARK: - Constructors

/**
 * Constructs a Network Controller
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

/**
 * Initializes a Network Controller. Returns true iff successful.
 */
bool NetworkController::init() {
    _http = SLNet::HTTPConnection2::GetInstance();
    _tcp = SLNet::TCPInterface::GetInstance();
    _tcp->AttachPlugin(_http);
    _tcp->Start(0, 0, 1);
    return true;
}

//  MARK: - Methods

/**
 * Returns the mapping of playerID to male boolean
 */
std::unordered_map<int, bool> NetworkController::getMales() {
    unordered_map<int, bool> result;
    for (int playerID = 0; playerID < 5; playerID++) {
        result[playerID] = _players[playerID].male;
    }
    return result;
}

/**
 * Establishes a host connection with the server
 */
bool NetworkController::connect() {
    _connection = NetworkConnection::alloc(_config);
    
    for (int playerID = 0; playerID < 5; playerID++) {
        _players[playerID] = {true, playerID, -2, 0, "None"};
    }
    _players[0].username = "Player 1";
    
    update(0);
    return _status != IDLE;
}

/**
 * Establishes a client connection with the server
 */
bool NetworkController::connect(const std::string room) {
    _connection = NetworkConnection::alloc(_config, room);
    
    for (int playerID = 0; playerID < 5; playerID++) {
        _players[playerID] = {true, playerID, -2, 0, "None"};
    }
    
    update(0);
    return _status != IDLE;
}

//  MARK: - Matchmaking

/**
 * Checks the connection and updates the status accordingly (pre-game)
 */
void NetworkController::update(float timestep) {
    if (_connection == nullptr) {
        _status = IDLE;
        return;
    }
    
    // Check the status of the network connection
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
    case NetworkConnection::NetStatus::NatFailure:
        _status = IDLE;
        break;
    }
    
    // Keep track of long it's been since the last player sent a ping
    for (int playerID = 0; playerID < getNumPlayers(); playerID++) {
        int myID = getPlayerID() ? *getPlayerID() : -1;
        if (myID != playerID) _players[playerID].lastPing += timestep;
    }
    
    // Let other players know that you're still active
    sendPing();
    
    // Let other players know who you are
    sendPlayer();
    
    // Receive data from other players
    for (int i = 0; i < NETWORK_STEP; i++) {
        _connection->receive([this](const std::vector<uint8_t> msg) {
            _deserializer.receive(msg);
            vector<float> data = _deserializer.readFloatVector();
            switch ((int) data.at(0)) {
            case PING:
                {
                int playerID = (int) data.at(1);
                if (playerID != -1) _players[playerID].lastPing = 0;
                break;
                }
            case PLAYER:
                {
                int playerID = (int) data.at(1);
                int myID = getPlayerID() ? *getPlayerID() : -1;
                if (playerID != -1 && playerID != myID) {
                    _players[playerID].male = _deserializer.readBool();
                    _players[playerID].playerID = (int) _deserializer.readFloat();
                    _players[playerID].playerNumber = (int) _deserializer.readFloat();
                    _players[playerID].username = _deserializer.readString();
                }
                break;
                }
            case START_GAME:
                _status = START;
                int i = 2;
                while (i < data.size()) {
                    int player = (int) data.at(i);
                    int role = (int) data.at(i+1);
                    if (player == this->getPlayerID()) _playerNumber = role;
                    _players[player].playerNumber = role;
                    i += 2;
                }
                for (int i = 0; i < 5; i++) {
                    _players[i].username = _deserializer.readString();
                }
                _level = _deserializer.readString();
                break;
            }
            _deserializer.reset();
        });
    }
}

/**
 * Sends a ping to verify that the player is still active
 */
void NetworkController::sendPing() {
    int playerID = getPlayerID() ? *getPlayerID() : -1;
    
    vector<float> data;
    data.push_back(PING);
    data.push_back(playerID);
    _serializer.writeFloatVector(data);
    _connection->send(_serializer.serialize());
    _serializer.reset();
}

/**
 * Sends the state of the lobby
 */
void NetworkController::sendPlayer() {
    int playerID = getPlayerID() ? *getPlayerID() : -1;
    
    vector<float> data;
    data.push_back(PLAYER);
    data.push_back(playerID);
    _serializer.writeFloatVector(data);
    
    _serializer.writeBool(_players[playerID].male);
    _serializer.writeFloat(_players[playerID].playerID);
    _serializer.writeFloat(_players[playerID].playerNumber);
    _serializer.writeString(_players[playerID].username);
    
    _connection->send(_serializer.serialize());
    _serializer.reset();
}

/**
 * Sends a byte vector to start the game
 */
void NetworkController::sendStartGame(string level, bool randomThief, int thiefChoice) {
    _level = level;
    vector<float> data;
    data.push_back(START_GAME);
    data.push_back(getNumPlayers());
    
    int count = 0;
    int thief = randomThief ? rand() % getNumPlayers() : thiefChoice;
    for (int playerID = 0; playerID < 5; playerID++) {
        _players[playerID].playerNumber = playerID == getPlayerID() ? -1 : count;
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
    
    for (int i = 0; i < 5; i++) {
        _serializer.writeString(_players[i].username);
    }
    
    _serializer.writeString(level);
    
    _connection->send(_serializer.serialize());
    _serializer.reset();
}

//  MARK: - Gameplay

/**
 * Checks the connection, updates the status accordingly, and updates the game (during game)
 */
void NetworkController::update(float timestep, std::shared_ptr<GameModel>& game) {
    // Give up if connection is not established
    if (_connection == nullptr) return;
    
    if (_connection->getStatus() != NetworkConnection::NetStatus::Connected) {
        // TODO: Handle other network statuses
        return;
    }
    
    // Let other players know that you're still active
    sendPing();
    
    // Receive data from other players
    for (int i = 0; i < NETWORK_STEP; i++) {
        _connection->receive([this, game](const std::vector<uint8_t> msg) {
            _deserializer.receive(msg);
            vector<float> data = _deserializer.readFloatVector();
            
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
                game->setThiefWon(_deserializer.readBool());
                break;
            default:
                break;
            }
            _deserializer.reset();
        });
    }
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
void NetworkController::sendGameOver(bool thiefWin) {
    if (_connection == nullptr) return;
    vector<float> data;
    data.push_back(GAME_OVER);
    
    _serializer.writeFloatVector(data);
    _serializer.writeBool(thiefWin);
    _connection->send(_serializer.serialize());
    _serializer.reset();
}

//  MARK: - Server

/**
 * Makes a request for a test endpoint
 */
void NetworkController::getTest() {
    makeGETrequest("/");
}

/**
 * Makes a request to check if the suggested roomID has been assigned a roomID
 */
void NetworkController::getRoom(string roomID) {
    makeGETrequest("/matchmaking/" + roomID);
}

/**
 * Makes a request to post a public room
 */
void NetworkController::postRoom(string roomID) {
    makePOSTrequest("/matchmaking/" + roomID, "");
}

/**
 * Makes a request to delete a public room
 */
void NetworkController::deleteRoom(string roomID) {
    makeDELETErequest("/matchmaking/" + roomID);
}

/**
 * Returns the content body of the response of a previously made request as a JsonValue
 */
shared_ptr<JsonValue> NetworkController::readResponse() {
    // Receive packets via the TCP Interface
    _tcp->HasCompletedConnectionAttempt();
    SLNet::Packet* packet = _tcp->Receive();
    while (packet) {
        _tcp->DeallocatePacket(packet);
        packet = _tcp->Receive();
    }
    _tcp->HasFailedConnectionAttempt();
    _tcp->HasLostConnection();

    // Create variables to store results of HTTP response
    SLNet::RakString stringTransmitted;
    SLNet::RakString hostTransmitted;
    SLNet::RakString responseReceived;
    SLNet::SystemAddress hostReceived;
    ptrdiff_t offset;
    
    // If there was an HTTP response received, store the results in variables
    if (_http->GetResponse(stringTransmitted,
                           hostTransmitted,
                           responseReceived,
                           hostReceived,
                           offset)) {
        // If the response isn't empty
        if (!responseReceived.IsEmpty()) {
            // If the response has content
            if (offset != -1) {
                // Get the content body
                string content = responseReceived.C_String() + offset;
                int begin = (int) content.find("{");
                int end = (int) content.find_last_of("}");
                
                // If the body is retrievable
                if (begin > -1 && begin < end && end < content.length()) {
                    string body = content.substr(begin, end + 1);
                    shared_ptr<JsonValue> json = JsonValue::allocWithJson(body);
                    return json;
                } else {
                    CULog("HTTP: Malformed JSON body");
                }
            } else {
                CULog("HTTP: No content body");
            }
        } else {
            CULog("HTTP: Response empty");
        }
    }
    return nullptr;
}

//  MARK: - Helpers

/**
 * Makes an HTTP request using the given RakString
 */
void NetworkController::makeRequest(SLNet::RakString request) {
    _http->TransmitRequest(request, WEBSERVER_ADDRESS, WEBSERVER_PORT);
}

/**
 * Makes a GET request at a given endpoint
 */
void NetworkController::makeGETrequest(string endpoint) {
    SLNet::RakString request = SLNet::RakString::FormatForGET(
        (WEBSERVER_ADDRESS + endpoint).c_str()
    );
    makeRequest(request);
}

/**
 * Makes a POST request at a given endpoint
 */
void NetworkController::makePOSTrequest(string endpoint, string body) {
    SLNet::RakString request = SLNet::RakString::FormatForPOST(
        (WEBSERVER_ADDRESS + endpoint).c_str(),
        "application/json",
        body.c_str()
    );
    makeRequest(request);
}

/**
 * Makes a PATCH request at a given endpoint
 */
void NetworkController::makePATCHrequest(string endpoint, string body) {
    
}

/**
 * Makes a DELETE request at a given endpoint
 */
void NetworkController::makeDELETErequest(string endpoint) {
    SLNet::RakString request = SLNet::RakString::FormatForDELETE(
        (WEBSERVER_ADDRESS + endpoint).c_str()
    );
    makeRequest(request);
}


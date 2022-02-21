//
//  CUNetworkConnection.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for an abstracted networking layer on top of Slikenet.
//  It utilizes a simple ad hoc lobby system inspired by Family Style and Sweet Space.
//  If you use this class, you should create your own lobby using the provided
//  docker container here:
//  
//  https://hub.docker.com/r/mtxing/cugl-nat-punchthrough
//  
//  This class uses our standard shared-pointer architecture.
//
//  1. The constructor does not perform any initialization; it just sets all
//     attributes to their defaults.
//
//  2. All initialization takes place via init methods, which can fail if an
//     object is initialized more than once.
//
//  3. All allocation takes place via static constructors which return a shared
//     pointer.
//
// Author: Michael Xing
// Version: 5/25/2021
// 
// Minor compatibility edits by Walker White (2/2/22).
//
// With help from onewordstudios:
// - Demi Chang
// - Aashna Saxena
// - Sam Sorenson
// - Michael Xing
// - Jeffrey Yao
// - Wendy Zhang
// https://onewordstudios.com/
// 
// With thanks to the students of CS 4152 Spring 2021
// for beta testing this class.
//
#ifndef __CU_NETWORK_CONNECTION_H__
#define __CU_NETWORK_CONNECTION_H__

#include <array>
#include <bitset>
#include <ctime>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_set>

#include <slikenet/BitStream.h>
#include <slikenet/MessageIdentifiers.h>
#include <slikenet/NatPunchthroughClient.h>

// Forward declarations
namespace SLNet {
    class RakPeerInterface;
}

namespace cugl {
/**
 * A class to support a connection to other players with a peer-to-peer interface.
 *
 * The premise of this class is to make networking as simple as possible. Simply call
 * {@link #send} with a byte vector, and then all others will receive it when they call 
 * {@link #receive}.  You can use {@link NetworkSerializer} and {@link NetworkDeserializer}
 * to handle more complex types.
 *
 * This class maintains a networked game using an ad-hoc server setup, but provides an 
 * interface that acts like it is peer-to-peer. The "host" is the server, and is the one 
 * all others are connected to. The "clients" are other players connected to the ad-hoc 
 * server. However, any messages sent are relayed by the host to all other players too, 
 * so the interface appears peer-to-peer.
 *
 * You can use this as a true client-server by just checking the player ID. Player ID 0
 * is the host, and all others are clients connected to the host.
 *
 * Using this class requires an external lobby server for NAT punchthrough. This server
 * only handles initial connections. It does not handle actual game data. This reduces
 * server costs significantly.  To get an external lobby server for your game, go to
 *
 *    https://hub.docker.com/r/mtxing/cugl-nat-punchthrough
 *
 * This class does support automatic reconnections, but does NOT support host migration.
 * If the host drops offline, the connection is closed.
 */
class NetworkConnection {
#pragma mark Support Structures
public:
    /**
     * The basic data needed to setup a connection.
     *
     * You must run an external NAT punchthrough server to use the connection
     * class.  To setup a NAT punchthrough server of your own, see:
     *
     *     https://github.com/mt-xing/nat-punchthrough-server
     */
    struct ConnectionConfig {
        /** Address of the NAT Punchthrough server */
        std::string punchthroughServerAddr;
        /** Port to connect on the NAT Punchthrough server */
        uint16_t punchthroughServerPort;
        /** Maximum number of players allowed per game (including host) */
        uint32_t maxNumPlayers;

        /**
         * The API version number.
         *
         * Clients with mismatched versions will be prevented from connecting to
         * each other. Start this at 0 and increment it every time a backwards
         * incompatible API change happens.
         */
        uint8_t apiVersion;

        /**
         * Creates a placeholder connection configuration
         */
        ConnectionConfig() {
            punchthroughServerAddr = "";
            punchthroughServerPort = 0;
            maxNumPlayers = 1;
        }

        /**
         * Creates a connection configuration for your game
         *
         * @param punchthroughServerAddr The URL for the punchthrough server
         * @param punchthroughServerPort The port for the punchthrough server
         * @param maxPlayers             The maximum number of players to support
         * @param apiVer                 The API version number for compatibility
         */
        ConnectionConfig(const std::string punchthroughServerAddr,
                         uint16_t punchthroughServerPort,
                         uint32_t maxPlayers, uint8_t apiVer) {
            this->punchthroughServerAddr = punchthroughServerAddr;
            this->punchthroughServerPort = punchthroughServerPort;
            this->maxNumPlayers = maxPlayers;
            this->apiVersion = apiVer;
        }
    };

    /**
     * Potential states the network connection could be in.
     */
    enum class NetStatus {
        /** No connection */
        Disconnected,
        /**
         * Waiting on a connection.
         *
         * If host, this means waiting on Room ID from server. If a client,  this
         * means waiting on Player ID from host
         */
        Pending,
        /**
         * Fully connected
         *
         * If host this means accepting connections. If a client, this means
         * successfully connected to host.
         */
        Connected,
        /**
         * Lost a connection and attempting to reconnect
         *
         * Failure causes a disconnection.
         */
        Reconnecting,
        /**
         *  The room ID does not exist, or room is already full
         */
        RoomNotFound,
        /**
         * API version numbers do not match..
         *
         * The match must be between host, client, AND the punchthrough server. When
         * running your own punchthrough server, you can specify a minimum API version
         * that your server will require, or else it will reject the connection.
         *
         * If you are using the demo server for the game lab, the minimum is 0.
         */
        ApiMismatch,
        /** Something unknown went wrong */
        GenericError
    };

private:
    /** Connection object */
    std::unique_ptr<SLNet::RakPeerInterface> _peer;
    /** Current status */
    NetStatus _status;
    /** API version number */
    uint8_t _apiVer;
    /** Number of players currently connected */
    uint8_t _numPlayers;
    /** Number of players connected when the game started */
    uint8_t _maxPlayers;
    /** Current player ID */
    std::optional<uint8_t> _playerID;
    /** Connected room ID */
    std::string _roomID;
    /** Which players are active */
    std::bitset<256> _connectedPlayers;
    /** Address of punchthrough server */
    std::unique_ptr<SLNet::SystemAddress> _natPunchServerAddress;
    /** NAT Punchthrough Client */
    SLNet::NatPunchthroughClient _natPunchthroughClient;
    /** Last reconnection attempt time, or none if n/a */
    std::optional<time_t> _lastReconnAttempt;
    /** Time when disconnected, or none if connected */
    std::optional<time_t> _disconnTime;
    /** The connection configuration for the protocol */
    ConnectionConfig _config;
    /** Whether to enable debug loggging on connection (default true) */
    bool _debug;

    /**
     * Data structure to manage the connection state for the server
     */
    struct HostPeers {
        /** Whether the game has started */
        bool started;
        /** Maximum number of players to allow in this game (NOT the max that was in this room) */
        uint32_t maxPlayers;
        /** Addresses of all connected players */
        std::vector<std::unique_ptr<SLNet::SystemAddress>> peers;
        /** Addresses of all players to reject */
        std::unordered_set<std::string> toReject;

        /**
         * Creates a connection data type for the server
         *
         * This version will support a maximum of 6 players.
         */
        HostPeers() : started(false), maxPlayers(6) {
            for (uint8_t i = 0; i < 5; i++) {
                peers.push_back(nullptr);
            }
        };

        /**
         * Creates a connection data type for the server
         *
         * @param max    The maximum number of allowed players
         */
        explicit HostPeers(uint32_t max) : started(false), maxPlayers(max) {
            for (uint8_t i = 0; i < max - 1; i++) {
                peers.push_back(nullptr);
            }
        };
    };

    /** Connection to host and room ID for client */
    struct ClientPeer {
        /** The address of the host server */
        std::unique_ptr<SLNet::SystemAddress> addr;
        /** The room id */
        std::string room;
        
        /**
         * Creates a connection for the given room id
         *
         * @param roomID The game room
         */
        explicit ClientPeer(std::string roomID) { 
            room = std::move(roomID); 
        }
    };

    /** Collection of peers for the host, or the host for clients */
    std::variant<HostPeers, ClientPeer> _remotePeer;

    /** Custom data packets for room connection */
    enum CustomDataPackets {
        Standard = 0,
        AssignedRoom,
        // Request to join, or success
        JoinRoom,
        // Couldn't find room
        JoinRoomFail,
        Reconnect,
        PlayerJoined,
        PlayerLeft,
        StartGame,
        DirectToHost
    };
    
#pragma mark Constructors
public:
    /**
     * Creates a degenerate network connection.
     *
     * The network connection has not yet initialized Slikenet and cannot be used.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on 
     * the heap, use one of the static constructors instead.
     */
    NetworkConnection();
    
    /**
     * Deletes this network connection, disposing all resources
     */
    ~NetworkConnection();
    
    /**
     * Disposes all of the resources used by this network connection.
     *
     * A disposed network connection can be safely reinitialized. 
     */
    void dispose();

    /**
     * Initializes a new network connection as host.
     *
     * This will automatically connect to the NAT punchthrough server and request a 
     * room ID.  This process is NOT instantaneous and the initializer will return 
     * true even without a guaranteed connection. Wait for {@link #getStatus} to 
     * return CONNECTED. Once it does, {@link #getRoomID} will return your assigned 
     * room ID.
     *
     * This method will return true if the Slikenet subsystem fails to initialize.
     *
     * @param setup Connection config
     *
     * @return true if initialization was successful
     */
    bool init(ConnectionConfig config);

    /**
     * Initializes a new network connection as a client.
     *
     * This will automatically connect to the NAT punchthrough server and then try
     * to connect to the host with the given ID. This process is NOT instantaneous 
     * and the initializer will return true even without a guaranteed connection.
     * Wait for {@link #getStatus} to return CONNECTED. Once it does, {@link #getPlayerID} 
     * will return your assigned player ID.
     *
     * @param setup     Connection config
     * @param roomID     Host's assigned Room ID
     *
     * @return true if initialization was successful
     */
    bool init(ConnectionConfig config, const std::string roomID);

    /**
     * Returns a newly allocated network connection as host.
     *
     * This will automatically connect to the NAT punchthrough server and request a 
     * room ID.  This process is NOT instantaneous and the initializer will return 
     * true even without a guaranteed connection. Wait for {@link #getStatus} to 
     * return CONNECTED. Once it does, {@link #getRoomID} will return your assigned 
     * room ID.
     *
     * This method will return true if the Slikenet subsystem fails to initialize.
     *
     * @param setup Connection config
     *
     * @return a newly allocated network connection as host.
     */
    static std::shared_ptr<NetworkConnection> alloc(ConnectionConfig config) {
        std::shared_ptr<NetworkConnection> result = std::make_shared<NetworkConnection>();
        return (result->init(config) ? result : nullptr);
    }

    /**
     * Returns a newly allocated network connection as a client.
     *
     * This will automatically connect to the NAT punchthrough server and then try
     * to connect to the host with the given ID. This process is NOT instantaneous 
     * and the initializer will return true even without a guaranteed connection.
     * Wait for {@link #getStatus} to return CONNECTED. Once it does, {@link #getPlayerID} 
     * will return your assigned player ID.
     *
     * @param setup     Connection config
     * @param roomID     Host's assigned Room ID
     *
     * @return a newly allocated network connection as a client.
     */
    static std::shared_ptr<NetworkConnection> alloc(ConnectionConfig config, const std::string roomID) {
        std::shared_ptr<NetworkConnection> result = std::make_shared<NetworkConnection>();
        return (result->init(config, roomID) ? result : nullptr);
    }


#pragma mark Main Networking Methods
    /**
     * Sends a byte array to all other players.
     *
     * Within a few frames, other players should receive this via a call to 
     * {@link #receive}.
     *
     * This requires a connection be established. Otherwise its behavior is undefined.
     *
     * You may choose to either send a byte array directly, or you can use the 
     * {@link NetworkSerializer} and {@link NetworkDeserializer} classes to encode 
     * more complex data.
     *
     * @param msg The byte array to send.
     */
    void send(const std::vector<uint8_t>& msg);

    /**
     * Sends a byte array to the host only.
     *
     * This is only useful when called from a client (player ID != 0). As host, this is
     * method does nothing. Within a few frames, the host should receive this via a call
     * to {@link #receive}
     *
     * This requires a connection be established. Otherwise its behavior is undefined.
     *
     * You may choose to either send a byte array directly, or you can use the
     * {@link NetworkSerializer} and {@link NetworkDeserializer} classes to encode
     * more complex data.
     *
     * @param msg The byte array to send.
     */
    void sendOnlyToHost(const std::vector<uint8_t>& msg);

    /**
     * Receives incoming network messages.
     *
     * This method must be called periodically EVEN BEFORE A CONNECTION IS ESTABLISHED.
     * Otherwise, the library has no way to receive and process incoming connections.
     *
     * When executed, the function `dispatch` willl be called on every received byte 
     * array since the last call to {@link #receive}. It is up to you to interpret
     * this data on your own or with {@link #NetworkDeserializer}
     *
     * A network frame can, but need not be, the same as a render frame. However,
     * during the network connection phase, before the game starts, this method should
     * be called every frame. Otherwise, the NAT Punchthrough library may fail. 
     * Afterwards, you can delay this to every few frames if necessary to relieve
     * congestion.
     *
     * @param dispatcher    The function to process received data
     */
    void receive(const std::function<void(const std::vector<uint8_t>&)>& dispatcher);

    /**
     * Marks the game as started and ban incoming connections except for reconnects.
     *
     * Note: This can only be called by the host. This method is ignored for players.
     */
    void startGame();

#pragma mark Attributes
    /**
     * Returns the current status of this network connection.
     *
     * @return the current status of this network connection.
     */
    NetStatus getStatus();

    /**
     * Returns the player ID or empty.
     *
     * If this player is the host, this is guaranteed to be 0, even before a connection 
     * is established. Otherwise, as client, this will return empty until connected to 
     * host and a player ID is assigned.
     *
     * @return the player ID or empty.
     */
    std::optional<uint8_t> getPlayerID() const { return _playerID; }

    /**
     * Returns the room ID or empty string.
     *
     * If this player is a client, this will return the room ID this object was 
     * constructed with. Otherwise, as host, this will return the empty string until 
     * connected to the punchthrough server and a room ID once it is assigned.
     *
     * @returns the room ID or empty string.
     */
    std::string getRoomID() const { return _roomID; }

    /**
     * Returns true if the given player ID is currently connected to the game.
     *
     * This does not return meaningful data until a connection is established.
     * This method is primarily designed for the host. However, a client can
     * test its connection status by using player ID 0.
     *
     * @param playerID    The player to test for connection
     *
     * @return true if the given player ID is currently connected to the game.
     */
    bool isPlayerActive(uint8_t playerID) {
        return _connectedPlayers.test(playerID);
    }

    /**
     * Returns the number of players currently connected to this game 
     *
     * This does not include any players that have been disconnected.
     *
     * @return the number of players currently connected to this game 
     */
    uint8_t getNumPlayers() const { return _numPlayers; }

    /**
     * Returns the number of players present when the game was started
     *
     * This includes any players that may have disconnected.
     *
     * @return the number of players present when the game was started
     */
    uint8_t getTotalPlayers() const { return _maxPlayers;  }
    
    /**
     * Returns the debug status of this network connection
     *
     * If the debug status is true, this connection will log verbose messages
     * during the initial handshake and any reconnection attempts. This value
     * should be set to false in a production system.
     *
     * @return the debug status of this network connection
     */
    bool getDebug() const { return _debug; }

    /**
     * Sets the debug status of this network connection
     *
     * If the debug status is true, this connection will log verbose messages
     * during the initial handshake and any reconnection attempts. This value
     * should be set to false in a production system.
     *
     * @param value The debug status of this network connection
     */
    void setDebug(bool value) { _debug = value; }

#pragma mark Communication Internals
private:
    /**
     * Broadcasts a message to everyone except the specified connection.
     *
     * PRECONDITION: This player MUST be the host
     *
     * @param packetType Packet type from RakNet
     * @param msg The message to send
     * @param ignore The address to not send to
     */
    void broadcast(const std::vector<uint8_t>& msg, 
                   SLNet::SystemAddress& ignore,
                   CustomDataPackets packetType = Standard);

    /**
     * Sends a message to all connected players
     * 
     * This method can be called by either connection.
     *
     * @param msg The message to send
     * @param ignore The address to not send to
     */
    void send(const std::vector<uint8_t>& msg, 
              CustomDataPackets packetType);

    /**
     * Sends a message to just one connection.
     *
     * @param msg The message to send
     * @param packetType The type of custom data packet
     * @param dest Desination address
     */
    void directSend(const std::vector<uint8_t>& msg, 
                    CustomDataPackets packetType, 
                    SLNet::SystemAddress dest);

    /**
     * Attempts to reconnect to the host.
     *
     * This method cannot be called by the host.  This method must be called
     * by the client when it is in the reconnecting phase. A successful connection 
     * must have previously been established.
     */
    void attemptReconnect();
    

#pragma mark Connection Handshake
    /*
    ===============================
     Connection Handshake Overview
    ===============================

            Host        Punchthrough Server            Client
            ====        ===================            ======
    c0        Connect ------------->
    ch1          <--------- Conn Req Accepted
              <--------- Room ID Assigned
    ch2        Accept Req

    c0                             <----------------- Connect
                         Conn Req Accepted ------------>
    cc1                             <----------------- Try connect to host
              <--------- Punch Succeeded -------------->
    cc2                                                Save host address
    cc3        Check hasRoom
            Connect ----------------------------------->
    cc4          <------------------------------------ Incoming connection
    cc5        Request Accepted -------------------------->
    cc6                                                Join Room
    
    */

    /** 
     * Step 0
     * 
     * Connect to punchthrough server (both client and host) 
     */
    void c0StartupConn();
    /** 
     * Host Step 1
     *
     * Server connection established 
     */
    void ch1HostConnServer(HostPeers& h);
    /** Host Step 2: Server gave room ID to host; awaiting incoming connections */
    void ch2HostGetRoomID(HostPeers& h, SLNet::BitStream& bts);

    /** 
     * Client Step 1:
     * 
     * Server connection established; request punchthrough to host from server 
     */
    void cc1ClientConnServer(ClientPeer& c);
    /** 
     * Client Step 2: 
     *
     * Client received successful punchthrough from server 
     */
    void cc2ClientPunchSuccess(ClientPeer& c, SLNet::Packet* packet);
    /** 
     * Client Step 3: 
     *
     * Host received successful punchthrough request passed through from server 
     */
    void cc3HostReceivedPunch(HostPeers& h, SLNet::Packet* packet);
    /** 
     * Client Step 4: 
     *
     * Client received direct connection request from host 
     */
    void cc4ClientReceiveHostConnection(ClientPeer& c, SLNet::Packet* packet);
    /** 
     * Client Step 5: 
     *
     * Host received confirmation of connection from client 
     */
    void cc5HostConfirmClient(HostPeers& h, SLNet::Packet* packet);
    /** 
     * Client Step 6: 
     *
     * Client received player ID from host and API 
     */
    void cc6ClientAssignedID(ClientPeer& c, const std::vector<uint8_t>& msgConverted);
    /** 
     * Client Step 7: 
     * 
     * Host received confirmation of game data from client; connection finished 
     */
    void cc7HostGetClientData(HostPeers& h, SLNet::Packet* packet, const std::vector<uint8_t>& msgConverted);

    /** 
     * Reconnect Step 1: 
     *
     * Picks up after client step 5; host sent reconn data to client 
     */
    void cr1ClientReceivedInfo(ClientPeer& c, const std::vector<uint8_t>& msgConverted);
    /** 
     * Reconnect Step 2: 
     * 
     * Host received confirmation of game data from client 
     */
    void cr2HostGetClientResp(HostPeers& h, SLNet::Packet* packet, const std::vector<uint8_t>& msgConverted);

};
}

#endif // __CU_NETWORK_CONNECTION_H__

//
//  CUNetworkConnection.cpp
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
#include <cugl/net/CUNetworkConnection.h>
#include <cugl/cugl.h>
#include <utility>


#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <fcntl.h>
#pragma comment(lib, "ws2_32")
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifndef _SSIZE_T_DEFINED
typedef int ssize_t;
#define _SSIZE_T_DEFINED
#endif
#ifndef _SOCKET_T_DEFINED
typedef SOCKET socket_t;
#define _SOCKET_T_DEFINED
#endif
#endif

#include <slikenet/peerinterface.h>

using namespace cugl;

// TODO: Reconfigure these as assignable attributes
/** How long to block on shutdown */
constexpr unsigned int SHUTDOWN_BLOCK = 10;

/** Length of room IDs */
constexpr uint8_t ROOM_LENGTH = 5;

/** How long to wait before considering ourselves disconnected (ms) */
constexpr size_t DISCONN_TIME = 5000;

/** How long to wait between reconnection attempts (seconds) */
constexpr size_t RECONN_GAP = 3;

/** How long to wait before giving up on reconnection (seconds) */
constexpr size_t RECONN_TIMEOUT = 15;


#pragma mark -
#pragma mark Vistor Utilities
/** Templates for the visitor pattern */
template <class... Fs>
struct overload;

template <class F0, class... Frest>
struct overload<F0, Frest...> : F0, overload<Frest...>
{
	overload(F0 f0, Frest... rest) : F0(f0), overload<Frest...>(rest...) {}

	using F0::operator();
	using overload<Frest...>::operator();
};

template <class F0>
struct overload<F0> : F0
{
	overload(F0 f0) : F0(f0) {}

	using F0::operator();
};

template <class... Fs>
auto make_visitor(Fs... fs)
{
	return overload<Fs...>(fs...);
}


#pragma mark -
#pragma mark Constructors
/**
 * Creates a degenerate network connection.
 *
 * The network connection has not yet initialized Slikenet and cannot be used.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
 * the heap, use one of the static constructors instead.
 */
NetworkConnection::NetworkConnection() :
_debug(true),
_apiVer(0),
_numPlayers(1),
_maxPlayers(1) {
    _status = NetStatus::GenericError;

}

/**
 * Deletes this network connection, disposing all resources
 */
NetworkConnection::~NetworkConnection() {
    _peer->Shutdown(SHUTDOWN_BLOCK);
    SLNet::RakPeerInterface::DestroyInstance(_peer.release());
}

/**
 * Disposes all of the resources used by this network connection.
 *
 * A disposed network connection can be safely reinitialized.
 */
void NetworkConnection::dispose() {
    _peer->Shutdown(SHUTDOWN_BLOCK);
    SLNet::RakPeerInterface::DestroyInstance(_peer.release());
}

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
bool NetworkConnection::init(ConnectionConfig config) {
    if (_peer) {
        return false;
    }
    _status = NetStatus::Pending;
    _apiVer = config.apiVersion;
    _numPlayers = 1;
    _maxPlayers = 1;
    _playerID = 0;
    _config = config;
    c0StartupConn();
	_remotePeer = HostPeers(config.maxNumPlayers);
    return _peer != nullptr;
}

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
bool NetworkConnection::init(ConnectionConfig config, std::string roomID) {
    if (_peer) {
        return false;
    }
    _status = NetStatus::Pending;
    _apiVer = config.apiVersion;
    _numPlayers = 1;
    _maxPlayers = 1;
    _playerID = 0;
    _config = config;
    c0StartupConn();
    _remotePeer = ClientPeer(std::move(roomID));
    if (_peer != nullptr) {
        _peer->SetMaximumIncomingConnections(1);
        return true;
    }
    return false;
}

#pragma mark -
#pragma mark Connection Handshake
/**
 * Read the message from a bitstream into a byte vector.
 *
 * This works if the BitStream was encoded in the standard format used by
 * this class.
 *
 * @param bts   The data bitstream
 *
 * @return the corresponding byte vector
 */
std::vector<uint8_t> readBs(SLNet::BitStream& bts) {
	uint8_t ignored;
	bts.Read(ignored);
	uint8_t length;
	bts.Read(length);

	std::vector<uint8_t> msgConverted;
	msgConverted.resize(length, 0);

	bts.ReadAlignedBytes(msgConverted.data(), length);
	return msgConverted;
}

/**
 * Step 0
 *
 * Connect to punchthrough server (both client and host)
 */
void NetworkConnection::c0StartupConn() {
	_peer = std::unique_ptr<SLNet::RakPeerInterface>(SLNet::RakPeerInterface::GetInstance());

	_peer->SetTimeoutTime(DISCONN_TIME, SLNet::UNASSIGNED_SYSTEM_ADDRESS);

	_peer->AttachPlugin(&(_natPunchthroughClient));
	_natPunchServerAddress = std::make_unique<SLNet::SystemAddress>(
		SLNet::SystemAddress(_config.punchthroughServerAddr.c_str(),
                             _config.punchthroughServerPort));

	// Use the default socket descriptor
	// This will make the OS assign us a random port.
	SLNet::SocketDescriptor socketDescriptor;
	// Allow connections for each player and one for the NAT server.
	_peer->Startup(_config.maxNumPlayers, &socketDescriptor, 1);

    if (_debug) {
        CULog("Your GUID is: %s",
            _peer->GetGuidFromSystemAddress(SLNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());
    }

	// Connect to the NAT Punchthrough server
    if (_debug) {
        CULog("Connecting to punchthrough server");
    }
	_peer->Connect(_natPunchServerAddress->ToString(false),
                   _natPunchServerAddress->GetPort(), nullptr, 0);
}

/**
 * Host Step 1
 *
 * Server connection established
 */
void NetworkConnection::ch1HostConnServer(HostPeers& h) {
    if (_debug) {
        CULog("Connected to punchthrough server; awaiting room ID");
    }
}

/**
 * Host Step 2
 *
 * Server gave room ID to host; awaiting incoming connections
 */
void cugl::NetworkConnection::ch2HostGetRoomID(HostPeers& h, SLNet::BitStream& bts) {
	auto msgConverted = readBs(bts);
	std::stringstream newRoomId;
	for (size_t i = 0; i < ROOM_LENGTH; i++) {
		newRoomId << static_cast<char>(msgConverted[i]);
	}
	_connectedPlayers.set(0);
	_roomID = newRoomId.str();
    if (_debug) {
        CULog("Got room ID: %s; Accepting Connections Now", _roomID.c_str());
    }
	_status = NetStatus::Connected;
}

/**
 * Client Step 1:
 *
 * Server connection established; request punchthrough to host from server
 */
void NetworkConnection::cc1ClientConnServer(ClientPeer& c) {
    if (_debug) {
        CULog("Connected to punchthrough server");
        CULog("Trying to connect to %s", c.room.c_str());
    }
	SLNet::RakNetGUID remote;
	remote.FromString(c.room.c_str());
	_natPunchthroughClient.OpenNAT(remote, *(_natPunchServerAddress));
}

/**
 * Client Step 2:
 *
 * Client received successful punchthrough from server
 */
void NetworkConnection::cc2ClientPunchSuccess(ClientPeer& c, SLNet::Packet* packet) {
	c.addr = std::make_unique<SLNet::SystemAddress>(packet->systemAddress);
}

/**
 * Client Step 3:
 *
 * Host received successful punchthrough request passed through from server
 */
void NetworkConnection::cc3HostReceivedPunch(HostPeers& h, SLNet::Packet* packet) {
	auto p = packet->systemAddress;
    if (_debug) {
        CULog("Host received punchthrough; curr num players %d",
              _peer->NumberOfConnections());
    }

	bool hasRoom = false;
	if (!h.started || _numPlayers < _maxPlayers) {
		for (uint8_t i = 0; i < h.peers.size(); i++) {
			if (h.peers.at(i) == nullptr) {
				hasRoom = true;
				h.peers.at(i) = std::make_unique<SLNet::SystemAddress>(p);
				break;
			}
		}
	}

	if (!hasRoom) {
		// Client is still waiting for a response at this stage,
		// so we need to connect to them first before telling them no.
		// Store address to reject so we know this connection is invalid.
		h.toReject.insert(p.ToString());
        if (_debug) {
            CULog("Client attempted to join but room was full");
        }
	}

    if (_debug) {
        CULog("Connecting to client now");
    }
	_peer->Connect(p.ToString(false), p.GetPort(), nullptr, 0);
}

/**
 * Client Step 4:
 *
 * Client received direct connection request from host
 */
void NetworkConnection::cc4ClientReceiveHostConnection(ClientPeer& c, SLNet::Packet* packet) {
	if (packet->systemAddress == *c.addr) {
        if (_debug) {
            CULog("Connected to host :D");
        }
	}
}

/**
 * Client Step 5:
 *
 * Host received confirmation of connection from client
 */
void NetworkConnection::cc5HostConfirmClient(HostPeers& h, SLNet::Packet* packet) {

	if (h.toReject.count(packet->systemAddress.ToString()) > 0) {
        if (_debug) {
            CULog("Rejecting player connection - bye :(");
        }

		h.toReject.erase(packet->systemAddress.ToString());

		directSend({}, JoinRoomFail, packet->systemAddress);

		_peer->CloseConnection(packet->systemAddress, true);
		return;
	}

	for (uint8_t i = 0; i < h.peers.size(); i++) {
		if (*h.peers.at(i) == packet->systemAddress) {
			uint8_t pID = i + 1;
            if (_debug) {
                CULog("Player %d accepted connection request", pID);
            }

			if (h.started) {
				// Reconnection attempt
				directSend({ static_cast<uint8_t>(_numPlayers + 1), _maxPlayers,pID, _apiVer },
                           Reconnect, packet->systemAddress);
			}
			else {
				// New player connection
				_maxPlayers++;
				directSend(
					{ static_cast<uint8_t>(_numPlayers + 1), _maxPlayers, pID, _apiVer },
					JoinRoom,
					packet->systemAddress
				);
			}
			break;
		}
	}

    if (_debug) {
        CULog("Host confirmed players; curr connections %d",
              _peer->NumberOfConnections());
    }
}

/**
 * Client Step 6:
 *
 * Client received player ID from host and API
 */
void NetworkConnection::cc6ClientAssignedID(ClientPeer& c,
                                            const std::vector<uint8_t>& msgConverted) {
	bool apiMatch = msgConverted[3] == _apiVer;
	if (!apiMatch) {
        if (_debug) {
            CULogError("API version mismatch; currently %d but host was %d",
                       _apiVer,
                       msgConverted[3]);
        }
		_status = NetStatus::ApiMismatch;
	} else {
		_numPlayers = msgConverted[0];
		_maxPlayers = msgConverted[1];
		_playerID = msgConverted[2];
		_status = NetStatus::Connected;
	}

	_peer->CloseConnection(*_natPunchServerAddress, true);

	directSend({ *_playerID, (uint8_t)(apiMatch ? 1 : 0) }, JoinRoom, *c.addr);
}

/**
 * Client Step 7:
 *
 * Host received confirmation of game data from client; connection finished
 */
void NetworkConnection::cc7HostGetClientData(HostPeers& h, SLNet::Packet* packet,
                                             const std::vector<uint8_t>& msgConverted) {
	for (uint8_t i = 0; i < h.peers.size(); i++) {
		if (*h.peers.at(i) == packet->systemAddress) {
			uint8_t pID = i + 1;
            if (_debug) {
                CULog("Host verifying player %d connection info", pID);
            }

			if (pID != msgConverted[0]) {
                if (_debug) {
                    CULog("Player ID mismatch; client reported id %d; disconnecting",
                          msgConverted[0]);
                }
				_peer->CloseConnection(packet->systemAddress, true);
				return;
			}

			if (msgConverted[1] == 0) {
                if (_debug) {
                    CULog("Client %d reported outdated API or other issue; disconnecting", pID);
                }
				_peer->CloseConnection(packet->systemAddress, true);
				return;
			}

            if (_debug) {
                CULog("Player id %d was successfully verified; connection handshake complete",
                      pID);
            }
			_connectedPlayers.set(pID);
			std::vector<uint8_t> joinMsg = { pID };
			broadcast(joinMsg, packet->systemAddress, PlayerJoined);
			_numPlayers++;

			return;
		}
	}

	// If we make it here, we somehow got a connection to an unknown address
    if (_debug) {
        CULogError("Unknown connection target; disconnecting");
    }
	_peer->CloseConnection(packet->systemAddress, true);
}

/**
 * Reconnect Step 1:
 *
 * Picks up after client step 5; host sent reconn data to client
 */
void cugl::NetworkConnection::cr1ClientReceivedInfo(ClientPeer& c,
                                                    const std::vector<uint8_t>& msgConverted) {
    if (_debug) {
        CULog("Reconnection Progress: Received data from host");
    }

	bool success = msgConverted[3] == _apiVer;
	if (!success) {
        if (_debug) {
            CULogError("API version mismatch; currently %d but host was %d",
                       _apiVer,
                       msgConverted[3]);
        }
		_status = NetStatus::ApiMismatch;
	} else if (_status != NetStatus::Reconnecting) {
        if (_debug) {
            CULogError("But we're not trying to reconnect. Failure.");
        }
		success = false;
	} else if (_playerID != msgConverted[2]) {
        if (_debug) {
            CULogError("Invalid reconnection target; we are player ID %d but host thought we were %d",
                       _playerID.has_value() ? *_playerID : -1, msgConverted[2]);
        }
		_status = NetStatus::Disconnected;
		success = false;
	} else {
        if (_debug) {
            CULog("Reconnection Progress: Connection OK");
        }
		_numPlayers = msgConverted[0];
		_maxPlayers = msgConverted[1];
		_playerID = msgConverted[2];
		_status = NetStatus::Connected;

		_lastReconnAttempt.reset();
		_disconnTime.reset();
	}
	_peer->CloseConnection(*_natPunchServerAddress, true);

	directSend({ 
		static_cast<uint8_t>(_playerID.has_value() ? *_playerID : 0),
		static_cast<uint8_t>(success ? 1 : 0)
	}, Reconnect, *c.addr);
}

/**
 * Reconnect Step 2:
 *
 * Host received confirmation of game data from client
 */
void NetworkConnection::cr2HostGetClientResp(HostPeers& h, SLNet::Packet* packet,
                                             const std::vector<uint8_t>& msgConverted) {
    if (_debug) {
        CULog("Host processing reconnection response");
    }
	cc7HostGetClientData(h, packet, msgConverted);
}

#pragma mark -
#pragma mark Communication Internals
/**
 * Broadcasts a message to everyone except the specified connection.
 *
 * PRECONDITION: This player MUST be the host
 *
 * @param packetType Packet type from RakNet
 * @param msg The message to send
 * @param ignore The address to not send to
 */
void NetworkConnection::broadcast(const std::vector<uint8_t>& msg,
                                  SLNet::SystemAddress& ignore,
                                  CustomDataPackets packetType) {
	SLNet::BitStream bs;
	bs.Write(static_cast<uint8_t>(ID_USER_PACKET_ENUM + packetType));
	bs.Write(static_cast<uint8_t>(msg.size()));
	bs.WriteAlignedBytes(msg.data(), static_cast<unsigned int>(msg.size()));
	_peer->Send(&bs, MEDIUM_PRIORITY, RELIABLE, 1, ignore, true);
}

/**
 * Sends a message to all connected players
 *
 * This method can be called by either connection.
 *
 * @param msg The message to send
 * @param ignore The address to not send to
 */
void NetworkConnection::send(const std::vector<uint8_t>& msg, CustomDataPackets packetType) {
	SLNet::BitStream bs;
	bs.Write(static_cast<uint8_t>(ID_USER_PACKET_ENUM + packetType));
	bs.Write(static_cast<uint8_t>(msg.size()));
	bs.WriteAlignedBytes(msg.data(), static_cast<unsigned int>(msg.size()));

	std::visit(make_visitor(
		[&](HostPeers& /*h*/) {
			_peer->Send(&bs, MEDIUM_PRIORITY, RELIABLE, 1, *_natPunchServerAddress, true);
		},
		[&](ClientPeer& c) {
			if (c.addr == nullptr) {
				return;
			}
			_peer->Send(&bs, MEDIUM_PRIORITY, RELIABLE, 1, *c.addr, false);
		}), _remotePeer);
}

/**
 * Sends a message to just one connection.
 *
 * @param msg The message to send
 * @param packetType The type of custom data packet
 * @param dest Desination address
 */
void cugl::NetworkConnection::directSend(const std::vector<uint8_t>& msg,
                                         CustomDataPackets packetType,
                                         SLNet::SystemAddress dest) {
	SLNet::BitStream bs;
	bs.Write(static_cast<uint8_t>(ID_USER_PACKET_ENUM + packetType));
	bs.Write(static_cast<uint8_t>(msg.size()));
	bs.WriteAlignedBytes(msg.data(), static_cast<unsigned int>(msg.size()));
	_peer->Send(&bs, MEDIUM_PRIORITY, RELIABLE, 1, dest, false);
}

/**
 * Attempts to reconnect to the host.
 *
 * This method cannot be called by the host.  This method must be called
 * by the client when it is in the reconnecting phase. A successful connection
 * must have previously been established.
 */
void NetworkConnection::attemptReconnect() {
	CUAssertLog(_disconnTime.has_value(), "No time for disconnect??");

	time_t now = time(nullptr);
	if (now - *_disconnTime > RECONN_TIMEOUT) {
        if (_debug) {
            CULog("Reconnection timed out; giving up");
        }
		_status = NetStatus::Disconnected;
		return;
	}

	if (_lastReconnAttempt.has_value()) {
		if (now - *_lastReconnAttempt < RECONN_GAP) {
			// Too soon after last attempt; abort
			return;
		}
	}

    if (_debug) {
        CULog("Attempting reconnection");
    }

	_peer->Shutdown(0);
	SLNet::RakPeerInterface::DestroyInstance(_peer.release());
	
	_lastReconnAttempt = now;
	_peer = nullptr;

	c0StartupConn();
	_peer->SetMaximumIncomingConnections(1);
}

#pragma mark -
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
void NetworkConnection::send(const std::vector<uint8_t>& msg) {
    send(msg, Standard);
}

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
void NetworkConnection::sendOnlyToHost(const std::vector<uint8_t>& msg) {
    std::visit(make_visitor(
        [&](HostPeers& /*h*/) {},
       [&](ClientPeer& c) {
            send(msg, DirectToHost);
        }), _remotePeer);
}


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
void NetworkConnection::receive(
	const std::function<void(const std::vector<uint8_t>&)>& dispatcher) {

	switch (_status) {
	case NetStatus::Reconnecting:
		attemptReconnect();
		if (_peer == nullptr) {
            if (_debug) {
                CULog("Peer null");
            }
			return;
		}
		break;
	case NetStatus::Disconnected:
	case NetStatus::GenericError:
	case NetStatus::ApiMismatch:
	case NetStatus::RoomNotFound:
		return;
	case NetStatus::Connected:
	case NetStatus::Pending:
		break;
	}


	SLNet::Packet* packet = nullptr;
	for (packet = _peer->Receive(); packet != nullptr;
		_peer->DeallocatePacket(packet), packet = _peer->Receive()) {
		SLNet::BitStream bts(packet->data, packet->length, false);

		switch (packet->data[0]) {
		case ID_CONNECTION_REQUEST_ACCEPTED:
			// Connected to some remote server
			if (packet->systemAddress == *(_natPunchServerAddress)) {
				// Punchthrough server
				std::visit(make_visitor(
					[&](HostPeers& h) { ch1HostConnServer(h); },
					[&](ClientPeer& c) { cc1ClientConnServer(c); }), _remotePeer);
			}
			else {
				std::visit(make_visitor(
					[&](HostPeers& h) { cc5HostConfirmClient(h, packet); },
					[&](ClientPeer& /*c*/) {
						CULogError(
							"A connection request you sent was accepted despite being client?");
					}), _remotePeer);
			}
			break;
		case ID_NEW_INCOMING_CONNECTION: // Someone connected to you
            if (_debug) {
                CULog("A peer connected");
            }
			std::visit(make_visitor(
				[&](HostPeers& /*h*/) { CULogError("How did that happen? You're the host"); },
				[&](ClientPeer& c) { cc4ClientReceiveHostConnection(c, packet); }), _remotePeer);
			break;
		case ID_NAT_PUNCHTHROUGH_SUCCEEDED: // Punchthrough succeeded
            if (_debug) {
                CULog("Punchthrough success");
            }
			std::visit(make_visitor(
				[&](HostPeers& h) { cc3HostReceivedPunch(h, packet); },
				[&](ClientPeer& c) { cc2ClientPunchSuccess(c, packet); }), _remotePeer);
			break;
		case ID_NAT_TARGET_NOT_CONNECTED:
			_status = NetStatus::GenericError;
			break;
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
		case ID_REMOTE_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
            if (_debug) {
                CULog("Received disconnect notification");
            }
			std::visit(make_visitor(
				[&](HostPeers& h) {
					for (uint8_t i = 0; i < h.peers.size(); i++) {
						if (h.peers.at(i) == nullptr) {
							continue;
						}
						if (*h.peers.at(i) == packet->systemAddress) {
							uint8_t pID = i + 1;
                            if (_debug) {
                                CULog("Lost connection to player %d", pID);
                            }
							std::vector<uint8_t> disconnMsg{ pID };
							h.peers.at(i) = nullptr;
							if (_connectedPlayers.test(pID)) {
								_numPlayers--;
								_connectedPlayers.reset(pID);
							}
							send(disconnMsg, PlayerLeft);

							if (_peer->GetConnectionState(packet->systemAddress) == SLNet::IS_CONNECTED) {
								_peer->CloseConnection(packet->systemAddress, true);
							}
							return;
						}
					}
				},
				[&](ClientPeer& c) {
					if (packet->systemAddress == *_natPunchServerAddress) {
                        if (_debug) {
                            CULog("Successfully disconnected from Punchthrough server");
                        }
					}
					if (packet->systemAddress == *c.addr) {
                        if (_debug) {
                            CULog("Lost connection to host");
                        }
						_connectedPlayers.reset(0);
						switch (_status) {
						case NetStatus::Pending:
							_status = NetStatus::GenericError;
							return;
						case NetStatus::Connected:
							_status = NetStatus::Reconnecting;
							_disconnTime = time(nullptr);
							return;
						case NetStatus::Reconnecting:
						case NetStatus::Disconnected:
						case NetStatus::RoomNotFound:
						case NetStatus::ApiMismatch:
						case NetStatus::GenericError:
							return;
						}
					}
				}), _remotePeer);

			break;
		case ID_NAT_PUNCHTHROUGH_FAILED:
		case ID_CONNECTION_ATTEMPT_FAILED:
		case ID_NAT_TARGET_UNRESPONSIVE: {
			CULogError("Punchthrough failure %d", packet->data[0]);

			_status = NetStatus::GenericError;
			bts.IgnoreBytes(sizeof(SLNet::MessageID));
			SLNet::RakNetGUID recipientGuid;
			bts.Read(recipientGuid);

			CULogError("Attempted punchthrough to GUID %s failed", recipientGuid.ToString());
			break;
		}
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			_status = NetStatus::RoomNotFound;
			break;

		// Begin Non-SLikeNet Reported Codes
		case ID_USER_PACKET_ENUM + Standard: {
			auto msgConverted = readBs(bts);
			dispatcher(msgConverted);

			std::visit(make_visitor(
				[&](HostPeers& /*h*/) { broadcast(msgConverted, packet->systemAddress); },
				[&](ClientPeer& c) {}), _remotePeer);

			break;
		}
        case ID_USER_PACKET_ENUM + DirectToHost: {
            auto msgConverted = readBs(bts);

            std::visit(make_visitor(
                [&](HostPeers& /*h*/) {
                    dispatcher(msgConverted);
                },
                [&](ClientPeer& c) {
                    CULogError("Received direct to host message as client");
                }), _remotePeer);

            break;
        }
        case ID_USER_PACKET_ENUM + AssignedRoom: {

			std::visit(make_visitor(
				[&](HostPeers& h) { ch2HostGetRoomID(h, bts); },
				[&](ClientPeer& c) {
                    if (_debug) {
                        CULog("Assigned room ID but ignoring");
                    }
                }), _remotePeer);

			break;
		}
		case ID_USER_PACKET_ENUM + JoinRoom: {
			auto msgConverted = readBs(bts);

			std::visit(make_visitor(
				[&](HostPeers& h) { cc7HostGetClientData(h, packet, msgConverted); },
				[&](ClientPeer& c) { cc6ClientAssignedID(c, msgConverted); }
			), _remotePeer);
			break;
		}
		case ID_USER_PACKET_ENUM + JoinRoomFail: {
            if (_debug) {
                CULog("Failed to join room");
            }
			_status = NetStatus::RoomNotFound;
			break;
		}
		case ID_USER_PACKET_ENUM + Reconnect: {
			auto msgConverted = readBs(bts);

			std::visit(make_visitor(
				[&](HostPeers& h) { cr2HostGetClientResp(h, packet, msgConverted); },
				[&](ClientPeer& c) { cr1ClientReceivedInfo(c, msgConverted); }),
                       _remotePeer);

			break;
		}
		case ID_USER_PACKET_ENUM + PlayerJoined: {
			auto msgConverted = readBs(bts);

			std::visit(make_visitor(
				[&](HostPeers& h) { CULogError("Received player joined message as host"); },
				[&](ClientPeer& c) {
					_connectedPlayers.set(msgConverted[0]);
					_numPlayers++;
					_maxPlayers++;
				}), _remotePeer);

			break;
		}
		case ID_USER_PACKET_ENUM + PlayerLeft: {
			auto msgConverted = readBs(bts);

			std::visit(make_visitor(
				[&](HostPeers& h) { CULogError("Received player left message as host"); },
				[&](ClientPeer& c) {
					_connectedPlayers.reset(msgConverted[0]);
					_numPlayers--;
				}), _remotePeer);
			break;
		}
		case ID_USER_PACKET_ENUM + StartGame: {
			startGame();
			break;
		}
		default:
            if (_debug) {
                CULog("Received unknown message: %d", packet->data[0]);
            }
			break;
		}
	}
}

/**
 * Marks the game as started and ban incoming connections except for reconnects.
 *
 * Note: This can only be called by the host. This method is ignored for players.
 */
void NetworkConnection::startGame() {
    if (_debug) {
        CULog("Starting Game");
    }
	std::visit(make_visitor([&](HostPeers& h) {
		h.started = true;
		broadcast({}, const_cast<SLNet::SystemAddress&>(SLNet::UNASSIGNED_SYSTEM_ADDRESS), StartGame);
		}, [&](ClientPeer& c) {}), _remotePeer);
	_maxPlayers = _numPlayers;
}

/**
 * Returns the current status of this network connection.
 *
 * @return the current status of this network connection.
 */
NetworkConnection::NetStatus NetworkConnection::getStatus() {
	return _status;
}

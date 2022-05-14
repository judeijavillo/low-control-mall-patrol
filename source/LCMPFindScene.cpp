//
//  LCMPRoomScene.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/27/22
//

#include <cugl/cugl.h>
#include <iostream>
#include <sstream>

#include "LCMPFindScene.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT  720

//  MARK: - Constructors

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
bool FindScene::init(const std::shared_ptr<cugl::AssetManager>& assets, std::shared_ptr<NetworkController>& network) {
    // Initialize the scene to a locked width
    Size dimen = Application::get()->getDisplaySize();
    dimen *= SCENE_HEIGHT/dimen.height;
    _offset = Vec2((dimen.width-SCENE_WIDTH)/2.0f,(dimen.height-SCENE_HEIGHT)/2.0f);
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // Save the asset manager
    _assets = assets;
    
    // Save the network controller
    _network = network;
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("find");
    scene->setContentSize(dimen);
    scene->doLayout(); // Repositions the HUD
    
    // Get the interactive UI elements that we need to access later
    _startgame = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("find_backdrop_join"));
    _startgame->setPositionX(SCENE_WIDTH/2 + _offset.x);
    _startgame->setAnchor(Vec2(0.5,0.5));
    _backout = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("find_backdrop_back"));
    _gameid = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("find_backdrop_status"));
    _gameid->setPositionX(SCENE_WIDTH/2 + _offset.x);
    _gameid->setAnchor(Vec2(0.5,0.5));
    _player = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("find_backdrop_players"));
    _player->setPositionX(SCENE_WIDTH/2 + _offset.x);
    _player->setAnchor(Vec2(0.5,0.5));
    _status = Status::IDLE;
    
    // Program the buttons
    _backout->addListener([this](const std::string& name, bool down) {
        if (down) {
            _status = Status::ABORT;
        }
    });

    _startgame->addListener([this](const std::string& name, bool down) {
        if (down) {
            startGame();
        }
    });

    addChild(scene);
    setActive(false);
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void FindScene::dispose() {
    if (_active) {
        removeAllChildren();
        _active = false;
    }
}

//  MARK: - Methods

/**
 * The method called to update the scene.
 *
 * We need to update this method to constantly talk to the server
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void FindScene::update(float timestep) {
    if (_network->isConnected() && _status != START && _status != ABORT) {
        _network->update();
        switch (_network->getStatus()) {
        case NetworkController::Status::IDLE:
        case NetworkController::CONNECTING:
            break;
        case NetworkController::WAIT:
            if (_status == IDLE) {
                _network->postRoom(_network->getRoomID());
                _status = WAIT;
            } else if (_status == WAIT) {
                if (_requestCooldown >= 5) {
                    _requestCooldown = 0;
                    _network->getRoom(_network->getRoomID());
                } else _requestCooldown += timestep;
                shared_ptr<JsonValue> response = _network->readResponse();
                if (response) {
                    int status = response->getInt("status", -1);
                    if (status == 2) {
                        bool hosting = response->getBool("hosting", true);
                        if (hosting) {
                            _status = HOST;
                        } else {
                            int roomID = response->getInt("assignment", 00000);
                            _network->connect(to_string(roomID));
                            _network->setHost(false);
                            _status = CLIENT;
                        }
                    }
                }
            } 
            break;
        case NetworkController::START:
            _status = START;
            break;
        case NetworkController::ABORT:
            _status = ABORT;
            break;
        }
        
        // Update the status message
        switch (_status) {
            case IDLE:
                _gameid->setText("Connecting to server", true);
                break;
            case WAIT:
                _gameid->setText("Waiting for room assignment", true);
                break;
            case HOST:
                _gameid->setText("You are hosting!", true);
                break;
            case CLIENT:
                _gameid->setText("You are joining!", true);
                break;
            default:
                break;
        }
        _gameid->setPosition(600, 500);
        
        string message = strtool::format("Waiting for players (%d/5)", _network->getNumPlayers());
        _player->setText(message);
        configureStartButton();
    }
}

/**
 * Sets whether the scene is currently active
 *
 * This method should be used to toggle all the UI elements.  Buttons
 * should be activated when it is made active and deactivated when
 * it is not.
 *
 * @param value whether the scene is currently active
 */
void FindScene::setActive(bool value) {
    if (isActive() != value) {
        Scene2::setActive(value);
        if (value) {
            _network->disconnect();
            _status = IDLE;
            configureStartButton();
            _backout->activate();
            connect();
        } else {
            _startgame->deactivate();
            _backout->deactivate();
            // If any were pressed, reset them
            _startgame->setDown(false);
            _backout->setDown(false);
        }
    }
}

//  MARK: - Helpers

/**
 * Updates the text in the given button.
 *
 * Techincally a button does not contain text. A button is simply a scene graph
 * node with one child for the up state and another for the down state. So to
 * change the text in one of our buttons, we have to descend the scene graph.
 * This method simplifies this process for you.
 *
 * @param button    The button to modify
 * @param text      The new text value
 */
void FindScene::updateText(const std::shared_ptr<scene2::Button>& button, const std::string text) {
//    auto label = std::dynamic_pointer_cast<scene2::Label>(button->getChildByName("up")->getChildByName("label"));
//    label->setText(text);
}

/**
 * Connects to the game server
 *
 * @return true if the connection was successful
 */
bool FindScene::connect() {
    _network->connect();
    return true;
}

/**
 * Reconfigures the start button for this scene
 *
 * This is necessary because what the buttons do depends on the state of the
 * networking.
 */
void FindScene::configureStartButton() {
    switch (_status) {
        case IDLE:
            _startgame->deactivate();
            break;
        case WAIT:
            _startgame->deactivate();
            break;
        case HOST:
            _startgame->activate();
            break;
        case CLIENT:
            _startgame->deactivate();
            break;
        case START:
            _startgame->deactivate();
            break;
        case ABORT:
            _startgame->deactivate();
            break;
    }
}

/**
 * Starts the game.
 *
 * This method is called once the requisite number of players have connected.
 * It locks down the room and sends a "start game" message to all other
 * players.
 */
void FindScene::startGame() {
    if (_network->isConnected()) {
        _status = Status::START;
        _network->deleteRoom(_network->getRoomID());
        _network->sendStartGame(LEVEL_ONE_FILE);
    }
}

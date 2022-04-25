//
//  LCMPHostScene.cpp
//  Low Control Mall Patrol
//
//  This class represents the scene for the host when creating a game. Normally
//  this class would be combined with the class for the client scene (as both
//  initialize the network controller).  But we have separated to make the code
//  a little clearer for this lab.
//
//  Originator: Walker White, Aidan Hobler
//  Author: Kevin Games
//  Version: 2/8/22
//

#include <cugl/cugl.h>
#include <iostream>
#include <sstream>

#include "LCMPHostScene.h"

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
bool HostScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
                     std::shared_ptr<NetworkController>& network,
                     std::shared_ptr<AudioController>& audio) {
    // Initialize the scene to a locked width
    Size dimen = Application::get()->getDisplaySize();
    dimen *= SCENE_HEIGHT/dimen.height;
    _offset = Vec2((dimen.width-SCENE_WIDTH)/2.0f,(dimen.height-SCENE_HEIGHT)/2.0f);
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // Save the references to managers and controllers
    _assets = assets;
    _network = network;
    _audio = audio;
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("host");
    scene->setContentSize(dimen);
    scene->doLayout(); // Repositions the HUD
    
    // Get the interactive UI elements that we need to access later
    _startgame = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("host_backdrop_create"));
    _startgame->setPositionX(SCENE_WIDTH/2);
    _startgame->setAnchor(Vec2(0.5,0.5));
    _backout = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("host_backdrop_back"));
    _gameid = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("host_backdrop_roomID"));
    _gameid->setPositionX(SCENE_WIDTH/2);
    _gameid->setAnchor(Vec2(0.5,0.5));
    _player = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("host_backdrop_players"));
    _player->setPositionX(SCENE_WIDTH/2);
    _player->setAnchor(Vec2(0.5,0.5));
    _status = Status::IDLE;
    
    // Program the buttons
    _backout->addListener([this](const std::string& name, bool down) {
        if (down) {
	    	_status = Status::ABORT;
            _audio->stopSfx(CLICK_SFX);
            _audio->playSfx(_assets, CLICK_SFX, 0);
        }
    });

    _startgame->addListener([this](const std::string& name, bool down) {
        if (down) {
            _status = Status::START;
            _audio->stopSfx(CLICK_SFX);
            _audio->playSfx(_assets, CLICK_SFX, 0);
        }
    });
    
    addChild(scene);
    setActive(false);
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void HostScene::dispose() {
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
void HostScene::update(float timestep) {
    if (_network->isConnected() && _status != START && _status != ABORT) {
        _network->update();
        switch (_network->getStatus()) {
            case NetworkController::Status::IDLE:
            case NetworkController::CONNECTING:
                _status = IDLE;
                break;
            case NetworkController::WAIT:
            {
                _status = WAIT;
                _gameid->setText(("Tell your friends this room code: " + _network->getRoomID()), true);
                _gameid->setPosition(Vec2(SCENE_WIDTH/2, 3*SCENE_HEIGHT/4) + _offset);
                _gameid->setAnchor(Vec2(0.5,0.5));
                break;
            }
            case NetworkController::START:
                _status = START;
                break;
            case NetworkController::ABORT:
                _status = ABORT;
                break;
        }
        _player->setText(strtool::format("Waiting for players: (%d/5)", _network->getNumPlayers()), true);
        _player->setPosition(Vec2(SCENE_WIDTH/2, SCENE_HEIGHT/2) + _offset);
        _player->setAnchor(Vec2(0.5,0.5));
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
void HostScene::setActive(bool value) {
    if (isActive() != value) {
        Scene2::setActive(value);
        if (value) {
            _network->disconnect();
            _status = IDLE;
            _startgame->activate();
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
void HostScene::updateText(const std::shared_ptr<scene2::Button>& button, const std::string text) {
//    auto label = std::dynamic_pointer_cast<scene2::Label>(button->getChildByName("up")->getChildByName("label"));
//    label->setText(text);
}

/**
 * Connects to the game server
 *
 * @return true if the connection was successful
 */
bool HostScene::connect() {
    _network->connect();
    return true;
}

/**
 * Reconfigures the start button for this scene
 *
 * This is necessary because what the buttons do depends on the state of the
 * networking.
 */
void HostScene::configureStartButton() {
//    updateText(_startgame, _status == IDLE ? "Waiting" : "Start Game");
//    _startgame->activate();
}

/**
 * Starts the game.
 *
 * This method is called once the requisite number of players have connected.
 * It locks down the room and sends a "start game" message to all other
 * players.
 */
void HostScene::startGame() {
    if (_network->isConnected()) {
        _status = Status::START;
    }
}

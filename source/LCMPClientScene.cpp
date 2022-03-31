//
//  LCMPClientScene.cpp
//  Low Control Mall Patrol
//
//  This class represents the scene for the client when joining a game. Normally
//  this class would be combined with the class for the client scene (as both
//  initialize the network controller).  But we have separated to make the code
//  a little clearer for this lab.
//
//  Originator: Walker White, Aidan Hobler
//  Author: Kevin Games
//  Version: 2/18/22
//

#include <cugl/cugl.h>
#include <iostream>
#include <sstream>

#include "LCMPClientScene.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT  720

//  MARK: - Constructors

/**
 * Initializes the controller contents, and starts the game
 *
 * The constructor does not allocate any objects or memory.  This allows
 * us to have a non-pointer reference to this controller, reducing our
 * memory allocation.  Instead, allocation happens in this method.
 *
 * @param assets    The (loaded) assets for this game mode
 *
 * @return true if the controller is initialized properly, false otherwise.
 */
bool ClientScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
                       std::shared_ptr<NetworkController>& network,
                       std::shared_ptr<AudioController>& audio) {
    // Initialize the scene to a locked width
    Size dimen = Application::get()->getDisplaySize();
    dimen *= SCENE_HEIGHT/dimen.height;
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // Save the references to managers and controllers
    _assets = assets;
    _network = network;
    _audio = audio;
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("client");
    scene->setContentSize(dimen);
    scene->doLayout(); // Repositions the HUD
    
    // Get interactive UI elements
    _startgame = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("client_center_start"));
    _backout = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("client_back"));
    _gameid = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("client_center_game_field_text"));
    _player = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("client_center_players_field_text"));
    _status = Status::IDLE;
    
    _backout->addListener([this](const std::string& name, bool down) {
        if (down) {
            _status = Status::ABORT;
            _audio->stopSfx(BACK_SFX);
            _audio->playSound(_assets, BACK_SFX, true, 0);
        }
    });

    _startgame->addListener([=](const std::string& name, bool down) {
        if (down) {
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
            // This will call the _gameid listener
            if (_gameid->hasFocus()) {
                _gameid->releaseFocus();
            } else {
                connect(_gameid->getText());
            }
        }
    });
    
    _gameid->addExitListener([this](const std::string& name, const std::string& value) {
        connect(value);
    });
    
    addChild(scene);
    setActive(false);
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void ClientScene::dispose() {
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
void ClientScene::update(float timestep) {
    if (_network->isConnected() && _status != START && _status != ABORT) {
        _network->update();
        switch (_network->getStatus()) {
        case NetworkController::Status::IDLE:
        case NetworkController::CONNECTING:
            _status = IDLE;
            break;
        case NetworkController::WAIT:
            _status = WAIT;
            break;
        case NetworkController::START:
            _status = START;
            break;
        case NetworkController::ABORT:
            _status = ABORT;
            break;
        }
        
        _player->setText(to_string(_network->getNumPlayers()));
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
void ClientScene::setActive(bool value) {
    if (isActive() != value) {
        Scene2::setActive(value);
        if (value) {
            _status = IDLE;
            _network->disconnect();
            _gameid->activate();
            _backout->activate();
            _player->setText("1");
            configureStartButton();
        } else {
            _gameid->deactivate();
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
 * Checks that the network connection is still active.
 *
 * Even if you are not sending messages all that often, you need to be calling
 * this method regularly. This method is used to determine the current state
 * of the scene.
 *
 * @return true if the network connection is still active.
 */
void ClientScene::updateText(const std::shared_ptr<scene2::Button>& button, const std::string text) {
    auto label = std::dynamic_pointer_cast<scene2::Label>(button->getChildByName("up")->getChildByName("label"));
    label->setText(text);

}

/**
 * Connects to the game server as specified in the assets file
 *
 * The {@link #init} method set the configuration data. This method simply uses
 * this to create a new {@Link NetworkConnection}. It also immediately calls
 * {@link #checkConnection} to determine the scene state.
 *
 * @param room  The room ID to use
 *
 * @return true if the connection was successful
 */
bool ClientScene::connect(const std::string room) {
    _network->connect(room);
    return true;
}

/**
 * Reconfigures the start button for this scene
 *
 * This is necessary because what the buttons do depends on the state of the
 * networking.
 */
void ClientScene::configureStartButton() {
    // THIS IS WRONG. FIX ME
    switch (_status) {
    case IDLE:
        _startgame->setDown(false);
        _startgame->activate();
        updateText(_startgame,"Start Game");
        break;
    case JOIN:
        _startgame->deactivate();
        updateText(_startgame,"Connecting");
    case WAIT:
        _startgame->deactivate();
        updateText(_startgame,"Waiting");
    default:
        break;
    }
}

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

/** Time between animation frames */
#define ANIMATION_SPEED         0.07f

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
    _startgame = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("host_backdrop_ready"));
    _genderButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("host_backdrop_gender"));
    _backout = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("host_backdrop_back"));
    _gameid = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("host_backdrop_roomID"));
    
    // The text fields showing the player names
    _player1 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("host_backdrop_thiefField_text"));
    _player2 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("host_backdrop_cop1Field_text"));
    _player3 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("host_backdrop_cop2Field_text"));
    _player4 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("host_backdrop_cop3Field_text"));
    _player5 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("host_backdrop_cop4Field_text"));
    _players.push_back(_player1);
    _players.push_back(_player2);
    _players.push_back(_player3);
    _players.push_back(_player4);
    _players.push_back(_player5);
    
    // The sprite nodes showing the characters
    _thiefNode = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("host_backdrop_thief_up"));
    _cop1Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("host_backdrop_cop1"));
    _cop2Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("host_backdrop_cop2"));
    _cop3Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("host_backdrop_cop3"));
    _cop4Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("host_backdrop_cop4"));
    _nodes.push_back(_thiefNode);
    _nodes.push_back(_cop1Node);
    _nodes.push_back(_cop2Node);
    _nodes.push_back(_cop3Node);
    _nodes.push_back(_cop4Node);
    
    // Initialize skins
    _skinKeys = {"cat_ears", "propeller_hat", "police_hat", "halo", "plant"};
    for (int i = 0; i < _skinKeys.size(); i++) {
        _skins.push_back(std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("host_backdrop_" + _skinKeys[i])));
        _skins[i]->setVisible(false);
        _skins[i]->setPosition(_thiefNode->getPosition() + Vec2(0, _thiefNode->getHeight()/2));
        _skins[i]->setAnchor(0.5, 0.5);
    }
    // No skin selected
    skinChoice = -1;
    
    _status = Status::IDLE;
    
    // Program the buttons
    _backout->addListener([this](const std::string& name, bool down) {
        if (down) {
	    	_status = Status::ABORT;
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
        }
    });

    _startgame->addListener([this](const std::string& name, bool down) {
        if (down) {
            _status = Status::START;
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
        }
    });
    
    _genderButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
            if (_network->isConnected()) _network->toggleGender();
        }
    });
    
    _player1->addExitListener([this](const std::string& name, const std::string& value) {
        if (_network->isConnected()) _network->setUsername(value);
    });
    
    // Set other attributes for animations
    _aniFrame = 0;
    _prevTime = 0;
    
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
    updateSkins(timestep);
    updateLobby(timestep);
    if (_network->isConnected() && _status != START && _status != ABORT) {
        _player2->setText(_network->getPlayer(1).username);
        _player3->setText(_network->getPlayer(2).username);
        _player4->setText(_network->getPlayer(3).username);
        _player5->setText(_network->getPlayer(4).username);
        
        _network->update();
        switch (_network->getStatus()) {
            case NetworkController::Status::IDLE:
            case NetworkController::CONNECTING:
                _status = IDLE;
                break;
            case NetworkController::WAIT:
            {
                _status = WAIT;
                _gameid->setText(("Share this room code: " + _network->getRoomID()), true);
                break;
            }
            case NetworkController::START:
                _status = START;
                break;
            case NetworkController::ABORT:
                _status = ABORT;
                break;
        }
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
            _genderButton->activate();
            connect();
        } else {
            _startgame->deactivate();
            _backout->deactivate();
            _player1->deactivate();
            _player2->deactivate();
            _player3->deactivate();
            _player4->deactivate();
            _player5->deactivate();
            _genderButton->deactivate();
            // If any were pressed, reset them
            _startgame->setDown(false);
            _backout->setDown(false);
            _genderButton->setDown(false);
        }
    }
}

//  MARK: - Helpers

/**
 * Connects to the game server
 *
 * @return true if the connection was successful
 */
bool HostScene::connect() {
    _network->connect();
    
    _player1->activate();
    _player1->setBackground(Color4("#ffffffff"));
    _player1->setText(_network->getPlayer(0).username);
    return true;
}

/**
 * Plays animations for the players
 */
void HostScene::updateLobby(float timestep) {
    // Update the lobby
    for (int playerID = 0; playerID < 5; playerID++) {
        NetworkController::Player player = _network->getPlayer(playerID);
        string key;
        if (playerID == 0) {
            key = player.male ? "ss_thief_idle_right" : "ss_thief_idle_right_f";
        } else {
            key = player.male ? "ss_cop_idle_right" : "ss_cop_idle_right_f";
            _players[playerID]->setText(player.username);
        }
        _nodes[playerID]->setTexture(_assets->get<Texture>(key));
    }
    
    // Update the frame accordingly
    _prevTime += timestep;
    if (_prevTime >= ANIMATION_SPEED) {
        _prevTime = 0;
        _aniFrame = (_aniFrame >= 3) ? 0 : _aniFrame + 1;
        _thiefNode->setFrame(_aniFrame);
        _cop1Node->setFrame(_aniFrame);
        _cop2Node->setFrame(_aniFrame);
        _cop3Node->setFrame(_aniFrame);
        _cop4Node->setFrame(_aniFrame);
    }
}

/**
 * Updates the player customizations
 */
void HostScene::updateSkins(float timestep) {
    if (skinChoice == -1) {
        
    }
    else if (skinChoice >= _skinKeys.size()) {
        
    }
}

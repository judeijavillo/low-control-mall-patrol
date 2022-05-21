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
/** The longest roomID allowed */
#define MAX_ROOM_ID_LENGTH 5
/** Time between animation frames */
#define ANIMATION_SPEED         0.07f

Vec2 ROOM_ID_LABEL_HOME = Vec2(512, 870);

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
                       std::shared_ptr<AudioController>& audio,
                       bool sixteenNineAspectRatio) {
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
    
    _sixteenNineAspectRatio = sixteenNineAspectRatio;
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("join");

    if (sixteenNineAspectRatio) {
        scene->setContentSize(dimen);
        scene->doLayout(); // Repositions the HUD
        
        // Get interactive UI elements
        _keypad = _assets->get<scene2::SceneNode>("join_backdrop_keypad");
        _genderButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join_backdrop_gender"));
        _genderNode = std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("join_backdrop_gender_up"));
        _startgame = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join_backdrop_join"));
        _backout = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join_backdrop_back"));
        _gameid = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("join_backdrop_keypad_roomID"));
        _info  = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("join_backdrop_instructions"));
        _donutFront = std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("join_backdrop_keypad_donutBoxFront"));
        
        // The text fields showing the player names
        _player1 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join_backdrop_thiefField_text"));
        _player2 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join_backdrop_cop1Field_text"));
        _player3 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join_backdrop_cop2Field_text"));
        _player4 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join_backdrop_cop3Field_text"));
        _player5 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join_backdrop_cop4Field_text"));
        
        // The sprite nodes showing the characters
        _thiefNode = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join_backdrop_thief_up"));
        _cop1Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join_backdrop_cop1_up"));
        _cop2Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join_backdrop_cop2_up"));
        _cop3Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join_backdrop_cop3_up"));
        _cop4Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join_backdrop_cop4_up"));
        for (int i = 1; i <= 4; i++) {
            _copButtons.push_back(std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join_backdrop_cop" + to_string(i))));
            _copButtons[i-1]->deactivate();
        }
    }
    else {
        scene = _assets->get<scene2::SceneNode>("join43");
        scene->setContentSize(dimen);
        scene->doLayout(); // Repositions the HUD

        // Get interactive UI elements
        _keypad = _assets->get<scene2::SceneNode>("join43_backdrop_keypad");
        _genderButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join43_backdrop_gender"));
        _genderNode = std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("join43_backdrop_gender_up"));
        _startgame = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join43_backdrop_join"));
        _backout = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join43_backdrop_back"));
        _gameid = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("join43_backdrop_roomID"));
        _info = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("join43_backdrop_instructions"));
        _donutFront = std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("join43_backdrop_keypad_donutBoxFront"));
        
        // The text fields showing the player names
        _player1 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join43_backdrop_thiefField_text"));
        _player2 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join43_backdrop_cop1Field_text"));
        _player3 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join43_backdrop_cop2Field_text"));
        _player4 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join43_backdrop_cop3Field_text"));
        _player5 = std::dynamic_pointer_cast<scene2::TextField>(_assets->get<scene2::SceneNode>("join43_backdrop_cop4Field_text"));
        
        // The sprite nodes showing the characters
        _thiefNode = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join43_backdrop_thief_up"));
        _cop1Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join43_backdrop_cop1_up"));
        _cop2Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join43_backdrop_cop2_up"));
        _cop3Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join43_backdrop_cop3_up"));
        _cop4Node = std::dynamic_pointer_cast<scene2::SpriteNode>(_assets->get<scene2::SceneNode>("join43_backdrop_cop4_up"));
        for (int i = 1; i <= 4; i++) {
            _copButtons.push_back(std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join43_backdrop_cop" + to_string(i))));
            _copButtons[i]->deactivate();
        }
    }
    
    // Maintain a mapping of player names for later
    _players.push_back(_player1);
    _players.push_back(_player2);
    _players.push_back(_player3);
    _players.push_back(_player4);
    _players.push_back(_player5);
    
    // Maintain a mapping of character nodes for later
    _nodes.push_back(_thiefNode);
    _nodes.push_back(_cop1Node);
    _nodes.push_back(_cop2Node);
    _nodes.push_back(_cop3Node);
    _nodes.push_back(_cop4Node);
    
    // Initialize skins
    _skinKeys = {"cat_ears_left", "propeller_hat_left", "police_hat_left", "halo_left", "plant_left"};
    for (int i = 0; i < _skinKeys.size(); i++) {
        _skins.push_back(std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("join_backdrop_" + _skinKeys[i])));
        _skins[i]->setAnchor(Vec2(1,0.5));
        _skins[i]->setVisible(false);
        _skins[i]->flipVertical(true);
    }
    // No skin selected
    skinChoice = -1;
    
    _status = Status::IDLE;
    
    // Attach listener to back button
    _backout->addListener([this](const std::string& name, bool down) {
        if (down) {
            _status = Status::ABORT;
//            _audio->stopSfx(CLICK_SFX);
//            _audio->playSound(_assets, CLICK_SFX, true, 0);
        }
    });
    
    _genderButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            if (_network->isConnected()) _network->toggleGender();
        }
    });
    
    for (int i = 0; i < 5; i++) {
        _players[i]->addExitListener([this](const std::string& name, const std::string& value) {
            if (_network->isConnected()) _network->setUsername(value);
        });
    }
    
    // Create all of the numbered buttons
    for (int i = 0; i < 10; i++) {
        string name = strtool::format("join_backdrop_keypad_button%d", i);
        if(!sixteenNineAspectRatio) name = strtool::format("join43_backdrop_keypad_button%d", i);
        shared_ptr<scene2::Button> button = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>(name));
        button->addListener([this, i](const std::string& name, bool down) { pressButton(name, down, i); });
        _keypadButtons.insert(button);
    }
    
    // Create the X button
    shared_ptr<scene2::Button> buttonX = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join_backdrop_keypad_buttonX"));
    if (!sixteenNineAspectRatio) buttonX = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join43_backdrop_keypad_buttonX"));
    buttonX->addListener([this](const std::string& name, bool down) {
        _gameid->setText("", false);
    });
    _keypadButtons.insert(buttonX);
    
    // Create the DEL button
    shared_ptr<scene2::Button> buttonDEL = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join_backdrop_keypad_buttonDEL"));
    if (!sixteenNineAspectRatio) buttonDEL = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("join43_backdrop_keypad_buttonDEL"));
    buttonDEL->addListener([this](const std::string& name, bool down) {
        if (down) {
            string text = _gameid->getText();
            _gameid->setText(text.substr(0, text.length() - 1), false);
        }
    });
    _keypadButtons.insert(buttonDEL);
    
    // Attach listener to join button
    _startgame->addListener([=](const std::string& name, bool down) {
        if (down) {
            _status = Status::WAIT;
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
            connect(_gameid->getText());
        }
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
        _network->update(timestep);
        switch (_network->getStatus()) {
        case NetworkController::Status::IDLE:
            _status = WAIT;
        case NetworkController::CONNECTING:
            _status = JOIN;
            break;
        case NetworkController::WAIT:
            if (_status == JOIN) {
                _network->setUsername("Player " + (_network->getPlayerID() ? to_string(*(_network->getPlayerID()) + 1) : ""));
                _info->setText("Connected!");
                showLobby(true);
            }
            _status = WAIT;
            break;
        case NetworkController::START:
            _status = START;
            break;
        case NetworkController::ABORT:
            _status = ABORT;
            break;
        }
    }
    
    switch (_status) {
    case IDLE:
        _info->setText("Enter a room code", true);
        _startgame->setDown(false);
        _startgame->activate();
        break;
    case JOIN:
        _info->setText("Connecting");
        break;
    case WAIT:
    {
        updateLobby(timestep);
        _startgame->deactivate();
        break;
    }
    default:
        break;
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
            _backout->activate();
            for (auto button = _keypadButtons.begin(); button != _keypadButtons.end(); button++) {
                (*button)->activate();
            }
            _gameid->setText("");
            _info->setText("Enter a room code", true);
            showLobby(false);
        } else {
            _startgame->deactivate();
            _backout->deactivate();
            for (auto button = _keypadButtons.begin(); button != _keypadButtons.end(); button++) {
                (*button)->deactivate();
            }
            for (int i = 0; i < 5; i++) _players[i]->deactivate();
            _genderButton->deactivate();
            // If any were pressed, reset them
            _genderButton->setDown(false);
            _startgame->setDown(false);
            _backout->setDown(false);
        }
    }
}

//  MARK: - Helpers

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
 * Helper for showing and hiding the lobby
 */
void ClientScene::showLobby(bool lobby) {
    _keypad->setVisible(!lobby);
    _startgame->setDown(lobby);
    lobby ? _startgame->deactivate() : _startgame->activate();
    _startgame->setVisible(!lobby);
    _genderButton->setVisible(lobby);
    for (int i = 0; i < 5; i++) {
        _players[i]->setVisible(lobby);
        _players[i]->deactivate();
        _players[i]->setBackground(Color4("#88888880"));
        _nodes[i]->setVisible(lobby);
    }
    if (!_network->isConnected()) return;
    int playerID = _network->getPlayerID() ? *(_network->getPlayerID()) : -1;
    if (playerID == -1) return;
    
    // Initialize cop button
    if (playerID > 0) {
        _copButtons[playerID-1]->activate();
        _copButtons[playerID-1]->addListener([this](const std::string& name, bool down) {
            if (down) {
                updateSkins();
            }
        });
        for (int i = 0; i < _skins.size(); i++) {
            _skins[i]->setPosition(_copButtons[playerID-1]->getPosition() + Vec2(-_copButtons[playerID-1]->getWidth()/8, _copButtons[playerID-1]->getHeight()/4));
        }
    }
        
    std::shared_ptr<cugl::scene2::TextField> player = _players[playerID];
    player->setText("Player " + to_string(playerID + 1));
    player->activate();
    player->setBackground(Color4("#ffffffff"));
    _genderButton->activate();
}

/**
 * Plays animations for the players and sets their names
 */
void ClientScene::updateLobby(float timestep) {
    for (int playerID = 0; playerID < 5; playerID++) {
        NetworkController::Player player = _network->getPlayer(playerID);
        string key;
        if (playerID == 0) {
            key = player.male ? "ss_thief_idle_right" : "ss_thief_idle_right_f";
        } else {
            key = player.male ? "ss_cop_idle_left" : "ss_cop_idle_left_f";
        }
        if (!_players[playerID]->isActive()) _players[playerID]->setText(player.username);
        _nodes[playerID]->setTexture(_assets->get<Texture>(key));
    }
    _genderNode->setTexture(_assets->get<Texture>(_network->getPlayer(*(_network->getPlayerID())).male ? "cop_head_f" : "cop_head"));
    
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
 * Helper for keypad button callback
 */
void ClientScene::pressButton(const std::string& name, bool down, int buttonID) {
    if (down) {
        if (_gameid->getText().length() < MAX_ROOM_ID_LENGTH) {
            _gameid->setText(_gameid->getText() + to_string(buttonID), false);
        }
    }
}

/**
 * Updates the player customizations
 */
void ClientScene::updateSkins() {
    if (skinChoice == _skinKeys.size()-1) {
        _skins[skinChoice]->setVisible(false);
        skinChoice = -1;
        if (_network->isConnected()) _network->setSkin("");
    }
    else {
        if (skinChoice != -1) {
            _skins[skinChoice]->setVisible(false);
        }
        skinChoice++;
        _skins[skinChoice]->setVisible(true);
        if (_network->isConnected()) _network->setSkin(_skinKeys[skinChoice]);
    }
}


//
//  LCMPHostScene.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/31/2022
//

#include <cugl/cugl.h>
#include <iostream>
#include <sstream>

#include "LCMPCustomizeScene.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT  720
#define MALE_COP                "ss_cop_idle_right"
#define FEMALE_COP              "ss_cop_idle_right_f"
#define MALE_COP_SELECT         "ss_cop_right"
#define FEMALE_COP_SELECT       "ss_cop_right_f"

#define THIEF_RUN_RIGHT         "ss_thief_right"
#define THIEF_RUN_RIGHT_F       "ss_thief_right_f"
#define MALE_CAT                "ss_thief_cat_right"
#define FEMALE_CAT              "ss_thief_cat_right_f"
#define MALE_HATLESS            "ss_thief_hatless_right"
#define FEMALE_HATLESS          "ss_thief_hatless_right_f"

#define DURATION 3.0f
#define DISTANCE 200
#define WALKPACE 50
#define CHOICE_COOLDOWN         0.5f


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
bool CustomizeScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
                     std::shared_ptr<NetworkController>& network,
                     std::shared_ptr<AudioController>& audio,
                    std::shared_ptr<scene2::ActionManager>& actions) {
    // Initialize the scene to a locked width
    Size dimen = Application::get()->getDisplaySize();
    dimen *= SCENE_HEIGHT/dimen.height;
    _dimen = dimen;
    _offset = Vec2((_dimen.width-SCENE_WIDTH)/2.0f,(_dimen.height-SCENE_HEIGHT)/2.0f);
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // Save the references to managers and controllers
    _assets = assets;
    _network = network;
    _audio = audio;
    _actions = actions;
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("customize");
    scene->setContentSize(dimen);
    scene->doLayout(); // Repositions the HUD
    
    // Get the interactive UI elements that we need to access later
    _startgame = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("customize_backdrop_start"));
    _startgame->setPositionX(SCENE_WIDTH/2 + _offset.x);
    _startgame->setAnchor(Vec2(0.5,0.5));
    _backout = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("customize_backdrop_back"));
    _title = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("customize_backdrop_title"));
    _title->setPositionX(SCENE_WIDTH/2 + _offset.x);
    _title->setAnchor(Vec2(0.5,0.5));
    
    _status = Status::IDLE;
    
    // Initialize custom assets
    _network->getPlayerNumber() == -1 ? _isThief = true : _isThief = false;
    
    // Initialize animation assets
    _keys = {THIEF_RUN_RIGHT, THIEF_RUN_RIGHT_F, MALE_CAT, FEMALE_CAT, MALE_HATLESS, FEMALE_HATLESS};
    
    // Initialize selected skin
    skin = 0;
    skinKey = _keys[skin];
    _didLeft = false;
    _customTime = 0.0;
    _lastChoice = -CHOICE_COOLDOWN;
    
    for (int i = 0; i < _keys.size(); i++) {
        _spriteSheets.push_back(assets->get<Texture>(_keys[i]));
    }
    
    _leftButton = std::dynamic_pointer_cast<scene2::Button>(assets->get<scene2::SceneNode>("customize_backdrop_left_button"));
    _leftButton->setPosition(Vec2(0, SCENE_HEIGHT/2) + _offset);
    _leftButton->setAnchor(Vec2(0.5,0.5));
    _rightButton = std::dynamic_pointer_cast<scene2::Button>(assets->get<scene2::SceneNode>("customize_backdrop_right_button"));
    _rightButton->setPosition(Vec2(SCENE_WIDTH, SCENE_HEIGHT/2) + _offset);
    _rightButton->setAnchor(Vec2(0.5,0.5));
    _aniFrame = 0;
    _prevTime = 0;
    
    for (int i = 0; i < _keys.size(); i++) {
        std::string key = "customize_" + _keys[i];
        _spriteNodes.push_back(std::dynamic_pointer_cast<scene2::SpriteNode>(assets->get<scene2::SceneNode>(key)));
        _spriteNodes[i]->setAnchor(Vec2::ANCHOR_CENTER);
        _spriteNodes[i]->setScale(0.5);
        _spriteNodes[i]->setVisible(false);
    }
    
    displaySkins(0);
    
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
            startGame();
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
        }
    });
    
    _leftButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            skin -= 1;
        }
    });
    
    _rightButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            skin += 1;
        }
    });
    
    addChild(scene);
    setActive(false);
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void CustomizeScene::dispose() {
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
void CustomizeScene::update(float timestep) {
    if (_network->isConnected() && _status != START && _status != ABORT) {
        _network->update();
        switch (_network->getStatus()) {
        case NetworkController::IDLE:
        case NetworkController::CONNECTING:
            displaySkins(timestep);
            _status = IDLE;
            break;
        case NetworkController::WAIT:
            displaySkins(timestep);
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
void CustomizeScene::setActive(bool value) {
    if (isActive() != value) {
        Scene2::setActive(value);
        if (value) {
            _status = IDLE;
            _backout->activate();
            _startgame->activate();
            _leftButton->activate();
            _rightButton->activate();
        } else {
            _startgame->deactivate();
            _backout->deactivate();
            _leftButton->deactivate();
            _rightButton->deactivate();
            // If any were pressed, reset them
            _startgame->setDown(false);
            _backout->setDown(false);
            _leftButton->setDown(false);
            _rightButton->setDown(false);
        }
    }
}

//  MARK: - Helpers

/**
 * Connects to the game server
 *
 * @return true if the connection was successful
 */
bool CustomizeScene::connect() {
    _network->connect();
    return true;
}

/**
 * Starts the game.
 *
 * This method is called once the requisite number of players have connected.
 * It locks down the room and sends a "start game" message to all other
 * players.
 */
void CustomizeScene::startGame() {
    if (_network->isConnected()) {
        _status = Status::START;
        _network->sendStartGame();
    }
}

/** Displays the skins */
void CustomizeScene::displaySkins(float timestep) {
    skin %= _keys.size();
    skinKey = _keys[skin];
    
    for (int i = 0; i < _keys.size(); i++) {
        _spriteNodes[i]->setScale(0.5);
        _spriteNodes[i]->setVisible(false);
    }
    
    _spriteNodes[skin]->setVisible(true);
    _spriteNodes[skin]->setPosition(Vec2(SCENE_WIDTH/2, SCENE_HEIGHT/2) + _offset);

    if (skin - 1 < 0) {
        _spriteNodes[_keys.size()-1]->setVisible(true);
        _spriteNodes[_keys.size()-1]->setPosition(Vec2(SCENE_WIDTH/4, SCENE_HEIGHT/2) + _offset);
    }
    else {
        _spriteNodes[skin-1]->setVisible(true);
        _spriteNodes[skin-1]->setPosition(Vec2(SCENE_WIDTH/4, SCENE_HEIGHT/2) + _offset);
    }
    
    if (skin + 1 == _keys.size()) {
        _spriteNodes[0]->setVisible(true);
        _spriteNodes[0]->setPosition(Vec2(3*SCENE_WIDTH/4, SCENE_HEIGHT/2) + _offset);
    }
    else {
        _spriteNodes[skin+1]->setVisible(true);
        _spriteNodes[skin+1]->setPosition(Vec2(3*SCENE_WIDTH/4, SCENE_HEIGHT/2) + _offset);
    }
    
    _prevTime += timestep;
    if (_prevTime >= 0.1) {
        _prevTime = 0;
        int frame = _spriteNodes[skin]->getFrame();
        frame >= 7 ? frame = 0 : frame++;
        _spriteNodes[skin]->setFrame(frame);
        skin - 1 < 0 ? _spriteNodes[_keys.size()-1]->setFrame(frame) : _spriteNodes[skin-1]->setFrame(frame);
        skin + 1 == _keys.size() ? _spriteNodes[0]->setFrame(frame) : _spriteNodes[skin+1]->setFrame(frame);
    }
}

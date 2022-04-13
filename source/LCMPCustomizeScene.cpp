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
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // Save the references to managers and controllers
    _assets = assets;
    _network = network;
    _audio = audio;
    _actions = actions;

    // Initialize the input controller
    _input.init(getBounds());
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("customize");
    scene->setContentSize(dimen);
    scene->doLayout(); // Repositions the HUD
    
    // Get the interactive UI elements that we need to access later
    _startgame = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("customize_center_start"));
    _backout = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("customize_back"));

    _status = Status::IDLE;
    
    // Initialize custom assets
    _network->getPlayerNumber() == -1 ? _isThief = true : _isThief = false;
    
    // Initialize animation assets
    _keys = {THIEF_RUN_RIGHT, THIEF_RUN_RIGHT_F, MALE_CAT, FEMALE_CAT, MALE_HATLESS, FEMALE_HATLESS};
    
    // Initialize selected skin
    skin = 0;
    skinKey = _keys[skin];
    _didLeft = nullptr;
    _customTime = 0.0;
    _lastChoice = -CHOICE_COOLDOWN;
    
    for (int i = 0; i < _keys.size(); i++) {
        _spriteSheets.push_back(assets->get<Texture>(_keys[i]));
    }
    
    std::vector<int> vec;
    for (int ii = 0; ii < 8; ii++) {
        vec.push_back(ii);
        for (int i = 0; i <_spriteSheets.size(); i++) {
            _animations.push_back(scene2::Animate::alloc(vec,DURATION));
        }
    }
    
    for (int i = 0; i < _keys.size(); i++) {
        _spriteNodes.push_back(scene2::SpriteNode::alloc(_spriteSheets[i], 1, 8));
        _spriteNodes[i]->setAnchor(Vec2::ANCHOR_CENTER);
        _spriteNodes[i]->setScale(0.5);
        _spriteNodes[i]->setPosition(Vec2(dimen.width/2, dimen.width/4));
        _spriteNodes[i]->setVisible(false);
        addChild(_spriteNodes[i]);
        _actions->activate(_keys[i], _animations[i], _spriteNodes[i]);
    }
    
//    _moveRight = cugl::scene2::MoveBy::alloc(Vec2(WALKPACE,0),DURATION);
//    _moveLeft = cugl::scene2::MoveBy::alloc(Vec2(WALKPACE,0),DURATION);
    
    displaySkins();
    
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
    
    addChild(scene);
    setActive(false);
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void CustomizeScene::dispose() {
    if (_active) {
        _input.clear();
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
        case NetworkController::Status::IDLE:
        case NetworkController::CONNECTING:
            updateInput(timestep);
            _status = IDLE;
            break;
        case NetworkController::WAIT:
            updateInput(timestep);
            _status = WAIT;
            break;
        case NetworkController::START:
            _status = START;
            break;
        case NetworkController::ABORT:
            _status = ABORT;
            break;
        }
        configureStartButton();
    }
}

/** Updates scene based on player input */
void CustomizeScene::updateInput(float timestep) {
    _customTime += timestep;
    if (_customTime >= _lastChoice + CHOICE_COOLDOWN) {
        _lastChoice = _customTime;
        
        _input.update(timestep);
        Vec2 swipe = _input.getSwipe();
        Vec2 movement = _input.getMovementVector(_isThief);
        bool didSwipe = _input.didSwipe();
        
        if (didSwipe) swipe.x < 0 ? skin -= 1 : skin += 1;
        if (didSwipe) swipe.x < 0 ? _didLeft = true : _didLeft = false;
        if (movement.x == 0) _didLeft = nullptr;
        if (movement.x != 0) movement.x < 0 ? skin -= 1 : skin += 1;
        if (movement.x != 0) movement.x < 0 ? _didLeft = true : _didLeft = false;
        
        skin %= _keys.size();
        skinKey = _keys[skin];
        displaySkins();
    }
    _actions->update(timestep);
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
void CustomizeScene::updateText(const std::shared_ptr<scene2::Button>& button, const std::string text) {
    auto label = std::dynamic_pointer_cast<scene2::Label>(button->getChildByName("up")->getChildByName("label"));
    label->setText(text);
}

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
 * Reconfigures the start button for this scene
 *
 * This is necessary because what the buttons do depends on the state of the
 * networking.
 */
void CustomizeScene::configureStartButton() {
    // THIS IS WRONG. FIX ME.
    updateText(_startgame, _status == IDLE ? "Waiting" : "Start Game");
    _startgame->activate();
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
void CustomizeScene::displaySkins() {
    for (int i = 0; i < _keys.size(); i++) {
        _spriteNodes[i]->setScale(0.5);
        _spriteNodes[i]->setPosition(Vec2(_dimen.width/2, _dimen.width/4));
        _spriteNodes[i]->setVisible(false);
    }
    
//    std::shared_ptr<cugl::scene2::MoveBy> move;
//    _didLeft ? move = _moveLeft : move = _moveRight;
    
    _spriteNodes[skin]->setScale(1.0);
    _spriteNodes[skin]->setVisible(true);
//    if (_actions->isActive(_keys[skin])) {
//        _actions->activate(_keys[skin], _animations[skin], _spriteNodes[skin]);
//        _actions->activate(_keys[skin], move,  _spriteNodes[skin]);
//    }
    
    if (skin - 1 < 0) {
        _spriteNodes[_keys.size()-1]->setVisible(true);
        _spriteNodes[_keys.size()-1]->setPosition(Vec2(_dimen.width/4, _dimen.width/4));
//        if (_actions->isActive(_keys[_keys.size()-1])) {
//            _actions->activate(_keys[_keys.size()-1], _animations[_keys.size()-1], _spriteNodes[_keys.size()-1]);
//        }
//        _actions->activate(_keys[_keys.size()-1], move,  _spriteNodes[_keys.size()-1]);
    }
    else {
        _spriteNodes[skin-1]->setVisible(true);
        _spriteNodes[skin-1]->setPosition(Vec2(_dimen.width/4, _dimen.width/4));
//        if (_actions->isActive(_keys[skin-1])) {
//            _actions->activate(_keys[skin-1], _animations[skin-1], _spriteNodes[skin-1]);
//        }
//        _actions->activate(_keys[skin-1], move,  _spriteNodes[skin-1]);
    }
    
    if (skin + 1 == _keys.size()) {
        _spriteNodes[0]->setVisible(true);
        _spriteNodes[0]->setPosition(Vec2(3*_dimen.width/4, _dimen.width/4));
//        if (_actions->isActive(_keys[0])) {
//            _actions->activate(_keys[0], _animations[0], _spriteNodes[0]);
//        }
//        _actions->activate(_keys[0], move,  _spriteNodes[0]);
    }
    else {
        _spriteNodes[skin+1]->setVisible(true);
        _spriteNodes[skin+1]->setPosition(Vec2(3*_dimen.width/4, _dimen.width/4));
//        if (_actions->isActive(_keys[skin+1])) {
//            _actions->activate(_keys[skin+1], _animations[skin+1], _spriteNodes[skin+1]);
//        }
//        _actions->activate(_keys[skin+1], move,  _spriteNodes[skin+1]);
    }
}

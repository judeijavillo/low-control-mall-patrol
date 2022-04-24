//
//  LCMPVictoryScene.cpp
//  Low Control Mall Patrol
//
//  This class provides the win condition logic.
//
//  Author: Kevin Games
//  Version: 4/23/22
//

#include <stdio.h>
#include <cugl/cugl.h>
#include <iostream>
#include <sstream>

#include "LCMPVictoryScene.h"

using namespace cugl;
using namespace std;

bool VictoryScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
                        std::shared_ptr<NetworkController>& network,
                        std::shared_ptr<AudioController>& audio,
                       std::shared_ptr<scene2::ActionManager>& actions,
                        bool thiefWin) {
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
    _actions = actions;
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("victory");
    scene->setContentSize(dimen);
    scene->doLayout(); // Repositions the HUD
    
    // Get the interactive UI elements that we need to access later
    _replayButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("victory_backdrop_replay"));
    _replayButton->setPositionX(SCENE_WIDTH/4);
    _replayButton->setAnchor(Vec2(0.5,0.5));
    _leaveButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("victory_backdrop_leave"));
    _leaveButton->setPositionX(3*SCENE_WIDTH/4);
    _leaveButton->setAnchor(Vec2(0.5,0.5));
    _text = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("victory_backdrop_text"));
    _text->setPositionX(SCENE_WIDTH/2);
    _text->setAnchor(Vec2(0.5,0.5));
    _status = Status::IDLE;
    
    // Program the buttons
    _replayButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _status = Status::START;
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
        }
    });

    _leaveButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _status = Status::ABORT;
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
        }
    });
    
    addChild(scene);
    setActive(false, _network->isHost(), thiefWin);
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void VictoryScene::dispose() {
    if (_active) {
        removeAllChildren();
        _active = false;
    }
}

void VictoryScene::updateMessage() {
    if (_isThief && _thiefWin) _text->setText("You Made Your Escape!");
    else if (_isThief) _text->setText("You Were Captured...");
    else if (_thiefWin) _text->setText("The Thief Escaped...");
    else _text->setText("You Captured The Thief!");
}

void VictoryScene::update(float timestep) {
    if (_network->isConnected() && _status != START && _status != ABORT) {
        _network->update();
        switch (_network->getStatus()) {
            case NetworkController::Status::IDLE:
            case NetworkController::CONNECTING:
                _status = IDLE;
                break;
            case NetworkController::WAIT:
                updateMessage();
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
void VictoryScene::setActive(bool value, bool isThief, bool thiefWin) {
    _isThief = isThief;
    _thiefWin = thiefWin;
    
    if (isActive() != value) {
        Scene2::setActive(value);
        if (value) {
            _status = IDLE;
            _replayButton->activate();
            _leaveButton->activate();
        } else {
            _replayButton->deactivate();
            _leaveButton->deactivate();
            // If any were pressed, reset them
            _replayButton->setDown(false);
            _leaveButton->setDown(false);
        }
    }
    Scene2::setActive(value);
}

/**
 * Connects to the game server
 *
 * @return true if the connection was successful
 */
bool VictoryScene::connect() {
    _network->connect();
    return true;
}

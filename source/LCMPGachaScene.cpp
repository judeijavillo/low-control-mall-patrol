//
//  LCMPGachaScene.cpp
//  Low Control Mall Patrol
//
//  This class provides the gacha logic.
//
//  Author: Kevin Games
//  Version: 5/4/22
//

#include <stdio.h>
#include <cugl/cugl.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>

#include "LCMPGachaScene.h"
#include "LCMPConstants.h"

using namespace cugl;
using namespace std;

bool GachaScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
                     std::shared_ptr<AudioController>& audio) {
    // Initialize the scene to a locked width
    Size dimen = Application::get()->getDisplaySize();
    dimen *= SCENE_HEIGHT/dimen.height;
    _dimen = dimen;
    _offset = Vec2((dimen.width-SCENE_WIDTH)/2.0f,(dimen.height-SCENE_HEIGHT)/2.0f);
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // Save the references to managers and controllers
    _assets = assets;
    _audio = audio;
    
    _audio->playSound(_assets, MENU_MUSIC, false, -1);
    
    _choice = NONE;
    
    _currency = 100;
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("gacha");
    scene->setContentSize(_dimen);
    scene->doLayout(); // Repositions the HUD
    addChild(scene);
    
    _backButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("gacha_settings_Back"));
    //_backButton->setPosition(Vec2(SCENE_WIDTH/16, 14*SCENE_HEIGHT/16) + _offset);
    //_backButton->setAnchor(Vec2(0.5,0.5));
    _rollButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("gacha_settings_Roll"));
    _rollButton->setPosition(Vec2(SCENE_WIDTH/2, SCENE_HEIGHT_ADJUST) + _offset);
    _rollButton->setAnchor(Vec2(0.5,0.5));
    _title = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("gacha_settings_Title"));
    _title->setPosition(Vec2(SCENE_WIDTH/2, SCENE_HEIGHT - SCENE_HEIGHT_ADJUST) + _offset);
    _title->setAnchor(Vec2(0.5,0.5));
    _normal = std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("gacha_settings_normal"));
    _normal->setPosition(Vec2(7*SCENE_WIDTH/16, 7*SCENE_HEIGHT/16) + _offset);
    _normal->setAnchor(Vec2(0.5,0.5));
    _normal->setVisible(false);
    _rare = std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("gacha_settings_rare"));
    _rare->setPosition(Vec2(7*SCENE_WIDTH/16, 7*SCENE_HEIGHT/16) + _offset);
    _rare->setAnchor(Vec2(0.5,0.5));
    _rare->setVisible(false);
    _donut = std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("gacha_settings_Donut"));
    _donut->setPosition(Vec2(14*SCENE_WIDTH/16, 12.5*SCENE_HEIGHT/16) + _offset);
    _donut->setAnchor(Vec2(0.5,0.5));
    _nfDeez = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("gacha_settings_Cash"));
    _nfDeez->setPosition(Vec2(13*SCENE_WIDTH/16, 14*SCENE_HEIGHT/16-10) + _offset);
    _nfDeez->setText(to_string(_currency));
    _nfDeez->setAnchor(Vec2(0.5,0.5));
    _cannotRoll = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("gacha_settings_None"));
    _cannotRoll->setPosition(Vec2(SCENE_WIDTH/2, SCENE_HEIGHT/2) + _offset);
    _cannotRoll->setAnchor(Vec2(0.5,0.5));
    _cannotRoll->setVisible(false);
   
     // Program the buttons
    _rollButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = GACHA;
        }
    });
    _backButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = BACK;
        }
    });

    setActive(false);
    return true;
}

//  MARK: - Methods

/**
 * Sets whether the scene is currently active
 *
 * This method should be used to toggle all the UI elements.  Buttons
 * should be activated when it is made active and deactivated when
 * it is not.
 *
 * @param value whether the scene is currently active
 */
void GachaScene::setActive(bool value) {
    if (isActive() != value) {
        if (value) {
            _choice = NONE;
        }
        Scene2::setActive(value);
        if (value) {
            _rollButton->activate();
            _backButton->activate();
        } else {
            _rollButton->deactivate();
            _backButton->deactivate();
            // If any were pressed, reset them
            _rollButton->setDown(false);
            _backButton->setDown(false);
        }
    }
}

void GachaScene::update(float timestep) {
    if (_choice == GACHA) {
        if (_currency < 10) {
            _cannotRoll->setVisible(true);
        }
        else {
            _currency -= 10;
            _nfDeez->setText(to_string(_currency));
            int match = rand() % 100 + 1;
            if (match >= 70) {
                _rare->setVisible(true);
                _normal->setVisible(false);
            }
            else {
                _normal->setVisible(true);
                _rare->setVisible(false);
            }
        }
        _choice = NONE;
    }
}

void GachaScene::dispose() {
    if (_active) {
        removeAllChildren();
        _active = false;
    }
}



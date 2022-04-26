//
//  LCMPShopScene.cpp
//  LowControlMallPatrol
//
//  Created by Alina Kim on 4/27/22.
//  Copyright © 2022 Game Design Initiative at Cornell. All rights reserved.
//

#include <stdio.h>
#include <cugl/cugl.h>
#include <iostream>
#include <sstream>

#include "LCMPShopScene.h"
#include "LCMPConstants.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT  720

bool ShopScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
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
    
    _prevTime = 0;
    _choice = NONE;
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("shop");
    scene->setContentSize(_dimen);
    scene->doLayout(); // Repositions the HUD
    addChild(scene);

    _keys = {"cat_ears", "propeller_hat", "police_hat", "halo", "plant"};
    
    _thief = scene2::SpriteNode::alloc(_assets->get<Texture>("ss_thief_idle_right"), 1, 4);
    _thief->setAnchor(Vec2::ANCHOR_CENTER);
    _thief->setPosition(Vec2(3*SCENE_WIDTH/4, SCENE_HEIGHT/2) + _offset);
    _thief->setVisible(true);
    addChild(_thief);
    
    for (string key : _keys) {
        _skins[key] = scene2::PolygonNode::allocWithTexture(_assets->get<Texture>(key));
        _skins[key]->setVisible(false);
        _skins[key]->setPosition(Vec2(3*SCENE_WIDTH/4, SCENE_HEIGHT/2 + _thief->getHeight()/2) + _offset);
        _skins[key]->setAnchor(0.5,0.5);
        addChild(_skins[key]);
        
        _buttons[key] = (std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("shop_settings_" + key)));
        _buttons[key]->setColor(Color4::GRAY);
        buttonListener(_buttons[key], key);
        
        _purchases[key] = false;
    }
    _backButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("shop_settings_Back"));
    _backButton->setPosition(Vec2(SCENE_WIDTH/16, 14*SCENE_HEIGHT/16) + _offset);
    _backButton->setAnchor(Vec2(0.5,0.5));
    _buyButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("shop_settings_Buy"));
    _buyButton->setPosition(Vec2(SCENE_WIDTH/2, SCENE_HEIGHT_ADJUST) + _offset);
    _buyButton->setAnchor(Vec2(0.5,0.5));
    _title = std::dynamic_pointer_cast<scene2::Label>(_assets->get<scene2::SceneNode>("shop_settings_Title"));
    _title->setPosition(Vec2(SCENE_WIDTH/2, SCENE_HEIGHT - SCENE_HEIGHT_ADJUST) + _offset);
    _title->setAnchor(Vec2(0.5,0.5));
    _selected = "";
   
     // Program the buttons
    _buyButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = BUY;
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

void ShopScene::buttonListener(std::shared_ptr<cugl::scene2::Button> button, string key) {
    button->addListener([this, button, key](const std::string& name, bool down) {
        if (down) {
            resetSkins();
            button->setScale(0.8);
            _selected = key;
            _purchases[key] = true;
            _skins[key]->setVisible(true);
        }
    });
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
void ShopScene::setActive(bool value) {
    if (isActive() != value) {
        Scene2::setActive(value);
        if (value) {
            for (string key : _keys) {
                _buttons[key]->activate();
            }
            _buyButton->activate();
            _backButton->activate();
        } else {
            for (string key : _keys) {
                _buttons[key]->deactivate();
            }
            _buyButton->deactivate();
            _backButton->deactivate();
            // If any were pressed, reset them
            for (string key : _keys) {
                _buttons[key]->setDown(false);
            }
            _buyButton->setDown(false);
            _backButton->setDown(false);
        }
    }
}

void ShopScene::update(float timestep) {
    if (_choice == BUY && _selected != "") {
        _buttons[_selected]->setColor(Color4::WHITE);
        _choice = NONE;
        _selected = "";
        
        if (!filetool::file_exists(Application::get()->getSaveDirectory() + "purchases.json")) {
            std::shared_ptr<cugl::JsonWriter> _writer = cugl::JsonWriter::alloc(Application::get()->getSaveDirectory() + "purchases.json");
            string s = "";
            for (string key : _keys) {
                s += "{\"" + key + "\":{" + to_string(_purchases[key]) + "}}";
            }
            _writer->write(s);
            _writer->close();
        }
    }
    
    _prevTime += timestep;
    if (_prevTime >= 0.1) {
        _prevTime = 0;
        int frame = _thief->getFrame();
        frame >= 3 ? frame = 0 : frame++;
        _thief->setFrame(frame);
    }
}

void ShopScene::dispose() {
    if (_active) {
        removeAllChildren();
        _active = false;
    }
}

void ShopScene::resetSkins() {
    for (string key : _keys) {
        _skins[key]->setScale(0.7);
        _skins[key]->setVisible(false);
    }
}


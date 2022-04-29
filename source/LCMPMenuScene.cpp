//
//  LCMPMenuScene.cpp
//  Low Control Mall Patrol
//
//  This class presents the initial menu scene.  It allows the player to
//  choose to be a host or a client.
//
//  Originator: Walker White, Aidan Hobler
//  Author: Kevin Games
//  Version: 2/18/22
//

#include <cugl/cugl.h>
#include <iostream>
#include <sstream>

#include "LCMPMenuScene.h"
#include "LCMPConstants.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT  720
#define ACT_KEY       "shop_menu"

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
bool MenuScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
                     std::shared_ptr<AudioController>& audio,
                     std::shared_ptr<cugl::scene2::ActionManager>& actions) {
    // Initialize the scene to a locked width
    Size dimen = Application::get()->getDisplaySize();
    _screenSize = dimen;
    dimen *= SCENE_HEIGHT/dimen.height;
    _dimen = dimen;
    _offset = Vec2((dimen.width-SCENE_WIDTH)/2.0f,(dimen.height-SCENE_HEIGHT)/2.0f);
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // Save the references to managers and controllers
    _assets = assets;
    _audio = audio;
    _actions = actions;
    
    _audio->playSound(_assets, MENU_MUSIC, false, -1);
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("menu");
    scene->setContentSize(_dimen);
    scene->doLayout(); // Repositions the HUD
    _choice = Choice::NONE;

    _hostbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu_backdrop_host"));
    _joinbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu_backdrop_join"));
    _findbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu_backdrop_find"));
    
    // Program the buttons
    _hostbutton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = Choice::HOST;
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
        }
    });
    _joinbutton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = Choice::JOIN;
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
        }
    });
    _findbutton->addListener([this](const std::string& name, bool down) {
        if (down) _choice = Choice::FIND;
    });
   
    initShop();
    
    std::shared_ptr<scene2::SceneNode> backdrop = _assets->get<scene2::SceneNode>("menu_backdrop");
    //_settings.init(backdrop, dimen, _offset, _assets, _actions);

    initSettingsButton();
    _didShop = false;
   
    scene->addChild(_shopMenu);
    _moveup = scene2::MoveTo::alloc(Vec2(_dimen.width/2, _shopMenu->getParent()->getHeight()*2), DROP_DURATION);
    _movedn = scene2::MoveTo::alloc(Vec2(_dimen.width/2, _shopMenu->getParent()->getHeight()), DROP_DURATION);
    _shopMenu->setPosition(Vec2(_dimen.width/2, _dimen.height*2));
    addChild(scene);
    setActive(false);
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void MenuScene::dispose() {
    if (_active) {
        removeAllChildren();
        _active = false;
    }
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
void MenuScene::setActive(bool value) {
    if (isActive() != value) {
        Scene2::setActive(value);
        if (value) {
            _choice = NONE;
            _hostbutton->activate();
            _joinbutton->activate();
            _findbutton->activate();
            _cat->deactivate();
            _propeller->deactivate();
            _police->deactivate();
            _halo->deactivate();
            _plant->deactivate();
            _shopButton->activate();
        } else {
            _hostbutton->deactivate();
            _joinbutton->deactivate();
            _findbutton->deactivate();
            _cat->deactivate();
            _propeller->deactivate();
            _police->deactivate();
            _halo->deactivate();
            _plant->deactivate();
            _shopButton->deactivate();
            // If any were pressed, reset them
            _hostbutton->setDown(false);
            _joinbutton->setDown(false);
            _findbutton->setDown(false);
            _cat->setDown(false);
            _propeller->setDown(false);
            _police->setDown(false);
            _halo->setDown(false);
            _plant->setDown(false);
            _shopButton->setDown(false);
        }
    }
}

void MenuScene::update(float timestep) {
    _actions->update(timestep);
    if (!_settings.didPause()) {
        updateShop();
    }
}


void MenuScene::doMove(const std::shared_ptr<scene2::MoveTo>& action) {
    if (_actions->isActive(ACT_KEY)) {
        CULog("You must wait for the animation to complete first");
    }
    else {
        auto fcn = EasingFunction::alloc(EasingFunction::Type::LINEAR);
        _actions->activate(ACT_KEY, action, _shopMenu, fcn);
    }
}

void MenuScene::initSettingsButton() {
    _settingsButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu_backdrop_settings"));
    _settingsButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _audio->stopSfx(CLICK_SFX);
            _audio->playSound(_assets, CLICK_SFX, true, 0);
            _settings.setDidPause(true);
        }
        });
    _settingsButton->activate();

}

void MenuScene::initShop() {
    _cat = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("shop_settings_Cat"));
    _propeller = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("shop_settings_Propeller"));
    _police = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("shop_settings_Police"));
    _halo = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("shop_settings_Halo"));
    _plant = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("shop_settings_Plant"));
    _shopCloseButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("shop_settings_X"));
    _shopButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu_backdrop_shop"));
    
    _shopMenu = _assets->get<scene2::SceneNode>("shop");
    _shopMenu->setContentSize(_dimen);
    _shopMenu->doLayout(); // Repositions the HUD
    _shopMenu->setAnchor(0.5, 1);
    _shopMenu->setVisible(true);
    
    _shopButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            doMove(_movedn);
            _didShop = true;
        }
    });
    
    // Properly sets position of the settings menu (by changing content size)
    Vec2 shopMenuPos = _shopMenu->getContentSize();
    shopMenuPos *= SCENE_HEIGHT / _dimen.height;
    _shopMenu->setContentSize(shopMenuPos);
   
     // Program the buttons
    _cat->addListener([this](const std::string& name, bool down) {
        if (down) {
            _cat->setScale(0.8);
            _propeller->setScale(0.7);
            _police->setScale(0.7);
            _halo->setScale(0.7);
            _plant->setScale(0.7);
        }
    });
    _propeller->addListener([this](const std::string& name, bool down) {
        if (down) {
            _propeller->setScale(0.8);
            _cat->setScale(0.7);
            _police->setScale(0.7);
            _halo->setScale(0.7);
            _plant->setScale(0.7);
        }
    });
    _police->addListener([this](const std::string& name, bool down) {
        if (down) {
            _police->setScale(0.8);
            _cat->setScale(0.7);
            _propeller->setScale(0.7);
            _halo->setScale(0.7);
            _plant->setScale(0.7);
        }
    });
    _halo->addListener([this](const std::string& name, bool down) {
        if (down) {
            _halo->setScale(0.8);
            _cat->setScale(0.7);
            _propeller->setScale(0.7);
            _police->setScale(0.7);
            _plant->setScale(0.7);
        }
    });
    _plant->addListener([this](const std::string& name, bool down) {
        if (down) {
            _plant->setScale(0.8);
            _cat->setScale(0.7);
            _propeller->setScale(0.7);
            _police->setScale(0.7);
            _halo->setScale(0.7);
        }
    });
    _shopCloseButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            doMove(_moveup);
            _didShop = false;
        }
    });
}

void MenuScene::updateShop() {
    if (_didShop) {
        _shopMenu->setVisible(true);
        _shopCloseButton->activate();
        _shopButton->deactivate();
        _hostbutton->deactivate();
        _joinbutton->deactivate();
        _findbutton->deactivate();
        _cat->activate();
        _cat->setVisible(true);
        _propeller->activate();
        _propeller->setVisible(true);
        _police->activate();
        _police->setVisible(true);
        _halo->activate();
        _halo->setVisible(true);
        _plant->activate();
        _plant->setVisible(true);
    }
    else {
        _shopCloseButton->deactivate();
        _shopButton->activate();
        _hostbutton->activate();
        _joinbutton->activate();
        _findbutton->activate();
        _cat->deactivate();
        _propeller->deactivate();
        _police->deactivate();
        _halo->deactivate();
        _plant->deactivate();
    }
}


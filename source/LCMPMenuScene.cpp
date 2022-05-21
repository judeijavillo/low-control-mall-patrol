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
                     std::shared_ptr<cugl::scene2::ActionManager>& actions,
                     bool sixteenNineAspectRatio) {
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
    
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("menu");
    scene->setContentSize(_dimen);
    scene->doLayout(); // Repositions the HUD
    _choice = Choice::NONE;

    // Acquire the scene built by the asset loader and resize it the scene
    if (sixteenNineAspectRatio) {
        _hostbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu_backdrop_host"));
        _joinbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu_backdrop_join"));
        _title = std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("menu_backdrop_title"));
        _shopButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu_backdrop_shop"));
        _gachaButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu_backdrop_profile"));
    }
    else {
        scene = _assets->get<scene2::SceneNode>("menu43");
        scene->setContentSize(_dimen);
        scene->doLayout(); // Repositions the HUD

        _hostbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu43_backdrop_host"));
        _joinbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu43_backdrop_join"));
        _title = std::dynamic_pointer_cast<scene2::PolygonNode>(_assets->get<scene2::SceneNode>("menu43_backdrop_title"));
        _shopButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu43_backdrop_shop"));
        _gachaButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("menu43_backdrop_profile"));
    }
    
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
    _shopButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = SHOP;
        }
    });
    _gachaButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = GACHA;
        }
    });

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
            _shopButton->activate();
            _gachaButton->activate();
        } else {
            _hostbutton->deactivate();
            _joinbutton->deactivate();
            _shopButton->deactivate();
            _gachaButton->deactivate();
            // If any were pressed, reset them
            _hostbutton->setDown(false);
            _joinbutton->setDown(false);
            _shopButton->setDown(false);
            _gachaButton->setDown(false);
        }
    }
}

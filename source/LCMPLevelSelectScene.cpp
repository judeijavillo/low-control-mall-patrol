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

#include "LCMPLevelSelectScene.h"
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
bool LevelSelectScene::init(const std::shared_ptr<cugl::AssetManager>& assets,
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
    
    _mapScreen = 0;
    _prevScreen = 0;

    //_audio->playSound(_assets, MENU_MUSIC, false, -1);
    
    // Acquire the scene built by the asset loader and resize it the scene
    std::shared_ptr<scene2::SceneNode> scene = _assets->get<scene2::SceneNode>("levelselect");
    scene->setContentSize(dimen);
    scene->doLayout(); // Repositions the HUD
    _choice = Choice::NONE;

    _map1button = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("levelselect_backdrop_directory_map1"));
    _map2button = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("levelselect_backdrop_directory_map2"));
    _map3button = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("levelselect_backdrop_directory_map3"));
    _map4button = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("levelselect_backdrop_directory_map4"));
    _nextbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("levelselect_backdrop_directory_next"));
    _prevbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("levelselect_backdrop_directory_prev"));
    _backbutton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("levelselect_backdrop_back"));
    
    _map3button->setVisible(false);
    _map4button->setVisible(false);
    _prevbutton->setVisible(false);
    _nextbutton->setVisible(false);

    // Program the buttons
    _map1button->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = Choice::ONE;
        }
    });
    _map2button->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = Choice::TWO;
        }
    });
    _map3button->addListener([this](const std::string& name, bool down) {
        if (down) _choice = Choice::THREE;
    });
    _map4button->addListener([this](const std::string& name, bool down) {
        if (down) {
            _choice = Choice::FOUR;
        }
        });
    //_nextbutton->addListener([this](const std::string& name, bool down) {
    //    if (down) {
    //        _mapScreen = 1;
    //    }
    //    });
    _prevbutton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _mapScreen = 0;
        }
        });
    _backbutton->addListener([this](const std::string& name, bool down) {
        if (down) _choice = Choice::BACK;
        });

    addChild(scene);
    setActive(false);
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void LevelSelectScene::dispose() {
    if (_active) {
        removeAllChildren();
        _active = false;
        _mapScreen = 0;
    }
}

void LevelSelectScene::update(float timestep) {
    if (_network->isConnected()) _network->update(timestep);
    if (_mapScreen != _prevScreen) {
        _prevScreen = _mapScreen;
        switch (_mapScreen) {
        case 0:
            prevPage();
            break;
        case 1:
            nextPage();
            break;
        default:
            break;
        }
    }
}

//  MARK: - Methods

void LevelSelectScene::nextPage() {
    _mapScreen = 1;
    _map1button->deactivate();
    _map2button->deactivate();
    _nextbutton->deactivate();
    _map1button->setVisible(false);
    _map2button->setVisible(false);
    _nextbutton->setVisible(false);

    _map3button->activate();
    _map4button->activate();
    _prevbutton->activate();
    _map3button->setVisible(true);
    _map4button->setVisible(true);
    _prevbutton->setVisible(true);
}

void LevelSelectScene::prevPage() {
    _mapScreen = 0;
    _map1button->activate();
    _map2button->activate();
    //_nextbutton->activate();
    _map1button->setVisible(true);
    _map2button->setVisible(true);
    //_nextbutton->setVisible(true);

    _map3button->deactivate();
    _map4button->deactivate();
    _prevbutton->deactivate();
    _map3button->setVisible(false);
    _map4button->setVisible(false);
    _prevbutton->setVisible(false);
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
void LevelSelectScene::setActive(bool value) {
    if (isActive() != value) {
        Scene2::setActive(value);
        if (value) {
            _choice = NONE;
            _map1button->activate();
            _map2button->activate();
            //_nextbutton->activate();
            _backbutton->activate();
        } else {
            _map1button->deactivate();
            _map2button->deactivate();
            _map3button->deactivate();
            _map4button->deactivate();
            _nextbutton->deactivate();
            _prevbutton->deactivate();
            _backbutton->deactivate();
            // If any were pressed, reset them
            _map1button->setDown(false);
            _map2button->setDown(false);
            _map3button->setDown(false);
            _map4button->setDown(false);
            _nextbutton->setDown(false);
            _prevbutton->setDown(false);
            _backbutton->setDown(false);
        }
    }
}

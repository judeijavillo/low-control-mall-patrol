//
//  LCMPPauseController.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/20/22
//

#include "LCMPPauseController.h"
#include "LCMPConstants.h"
#include "LCMPAudioController.h"

using namespace std;
using namespace cugl;

//  MARK: - Constants

/** The key for the settings button texture */
#define SETTINGS_BUTTON_TEXTURE    "settings_button"


//  MARK: - Constructors

/**
 * Disposes of all resources in this instance of UI Controller
 */
void PauseController::dispose() {
    _settingsMenu->dispose();
}

/**
 * Initializes a UI Controller
 */
bool PauseController::init(const shared_ptr<scene2::SceneNode> sceneNode,
                        Size screenSize,
                        Vec2 offset,
                        const std::shared_ptr<cugl::AssetManager>& assets,
                        const std::shared_ptr<cugl::scene2::ActionManager>& actions,
                        const std::shared_ptr<AudioController> audio) {
    
    // Save properties
    _sceneNode = sceneNode;
    _assets = assets;
    _screenSize = screenSize;
    _offset = offset;
    _actions = actions;
    _audio = audio;
    
    // Initialize booleans
    _didQuit = false;
    _didPause = false;

    // Initialize transparency
    _transparent = Color4(255, 255, 255, (int)(255 * 0.75));

    // Set up dimen
    _dimen = _screenSize;
    _dimen *= SCENE_HEIGHT / _dimen.height;
    
    // Taken from initSettings
      // Initialize into the main settings menu
    _currSettingsState = State::MAIN;
    _prevSettingsState = State::MAIN;

    // Set references to each menu
    _settingsMainMenu = std::dynamic_pointer_cast<scene2::SceneNode>(_assets->get<scene2::SceneNode>("pause_settings_MainMenu"));
    _settingsSoundMenu = std::dynamic_pointer_cast<scene2::SceneNode>(_assets->get<scene2::SceneNode>("pause_settings_SoundMenu"));

    // Set main settings menu references
    _soundsButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("pause_settings_MainMenu_SoundButton"));
    _statsButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("pause_settings_MainMenu_StatsButton"));
    _quitButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("pause_settings_MainMenu_QuitButton"));

    // Set sound menu references
    _musicSlider = std::dynamic_pointer_cast<scene2::Slider>(_assets->get<scene2::SceneNode>("pause_settings_SoundMenu_MusicSlider"));
    _SFXSlider = std::dynamic_pointer_cast<scene2::Slider>(_assets->get<scene2::SceneNode>("pause_settings_SoundMenu_SFXSlider"));

    _backButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("pause_settings_BackButton"));
    _closeButton = std::dynamic_pointer_cast<scene2::Button>(_assets->get<scene2::SceneNode>("pause_settings_X"));

    // Initialize settings button from assets manager
    //_settingsButtonNode = _assets->get<scene2::SceneNode>("game");
    //_settingsButtonNode->setContentSize(_screenSize);
    //Vec2 settingsButtonPos = _settingsButtonNode->getContentSize();
    //settingsButtonPos *= SCENE_HEIGHT / _screenSize.height;
    //_settingsButtonNode->setContentSize(settingsButtonPos);
    //_settingsButtonNode->doLayout(); // Repositions the HUD

    // Initialize settings menu from assets manager
    _settingsMenu = _assets->get<scene2::SceneNode>("pause");
    _settingsMenu->setContentSize(_screenSize);
    _settingsMenu->doLayout(); // Repositions the HUD

    // Properly sets position of the settings menu (by changing content size)
    Vec2 settingsMenuPos = _settingsMenu->getContentSize();
    settingsMenuPos *= SCENE_HEIGHT / _screenSize.height;
    _settingsMenu->setContentSize(settingsMenuPos);

    // Program the buttons
    //_settingsButton->addListener([this](const std::string& name, bool down) {
    //    if (down) {
    //        _didPause = true;
    //        doMove(_movedn);
    //    }
    //    });
    _soundsButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _currSettingsState = State::SOUND;
        }
        });
    _quitButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _didQuit = true;
        }
        });
    _closeButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _currSettingsState = State::MAIN;
            _didPause = false;
            _isPaused = false;
            doMove(_moveup);
        }
        });
    _backButton->addListener([this](const std::string& name, bool down) {
        if (down) {
            _currSettingsState = State::MAIN;
        }
        });

    _musicSlider->addListener([this](const std::string& name, float value) {
        if (value != _audio->getMusicMult()) {
            _audio->setMusicMult(value);
        }
        });
    _SFXSlider->addListener([this](const std::string& name, float value) {
        if (value != _audio->getSFXMult()) {
            _audio->setSFXMult(value);
        }
        });

    // Set visibility
    _settingsMenu->setVisible(true);
    _settingsSoundMenu->setVisible(false);
    _backButton->setVisible(false);

    _sceneNode->addChild(_settingsMenu);
    //_sceneNode->addChild(_settingsButtonNode);

    // Set proper position of settings menu when the game begins
    _settingsMenu->setPosition(Vec2(0, _dimen.height));


    // Settings menu movement
    _moveup = scene2::MoveTo::alloc(Vec2(0, _dimen.height), DROP_DURATION);
    _movedn = scene2::MoveTo::alloc(Vec2(0, _dimen.height * MENU_OFFSET), DROP_DURATION);
    
    _settingsMenu->setPosition(Vec2(0, _dimen.height));

    return true;
}

//  MARK: - Methods

/**
 * Updates the UI Controller
 */
void PauseController::update(float timestep) {
    
    // Display and activate correct buttons depending on pause state.
    if (_didPause) {
        if (!_isPaused) {
            _settingsMenu->setVisible(true);
            _soundsButton->activate();
            _quitButton->activate();
            _closeButton->activate();
            doMove(_movedn);
            //_settingsButton->deactivate();
            _isPaused = true;
            //_settingsButton->setDown(false);
        }
    }
    else {
        _soundsButton->deactivate();
        _quitButton->deactivate();
        _closeButton->deactivate();
        //_settingsButton->activate();
        _isPaused = false;
    }
    if (_currSettingsState != _prevSettingsState) {
        _settingsMainMenu->setVisible(false);
        _settingsSoundMenu->setVisible(false);
        _soundsButton->deactivate();
        _quitButton->deactivate();
        _musicSlider->deactivate();
        _SFXSlider->deactivate();
        _backButton->deactivate();
        _backButton->setVisible(false);
        switch (_currSettingsState)
        {
        case State::MAIN:
            _settingsMainMenu->setVisible(true);
            _soundsButton->activate();
            _quitButton->activate();
            _closeButton->activate();
            _backButton->setDown(false);
            break;
        case State::SOUND:
            _settingsSoundMenu->setVisible(true);
            _musicSlider->activate();
            _SFXSlider->activate();
            _backButton->activate();
            _backButton->setVisible(true);
            _soundsButton->setDown(false);
            break;
        case State::STATS:
            break;
        case State::CALIBRATE:
            break;
        default:
            break;
        }
        _prevSettingsState = _currSettingsState;
    }
}


void PauseController::doMove(const std::shared_ptr<scene2::MoveTo>& action) {
    if (_actions->isActive(SETTINGS_ACT_KEY)) {
        CULog("You must wait for the animation to complete first");
    }
    else {
        auto fcn = EasingFunction::alloc(EasingFunction::Type::LINEAR);
        _actions->activate(SETTINGS_ACT_KEY, action, _settingsMenu, fcn);
    }
}

//
//  LCMPPauseController.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 4/28/22
//

#ifndef __LCMP_PAUSE_CONTROLLER_H__
#define __LCMP_PAUSE_CONTROLLER_H__
#include <cugl/cugl.h>
#include "LCMPGameModel.h"
#include "LCMPAudioController.h"

class PauseController {
//  MARK: - Properties
// 
    // References to the settings menu

    /** The scenenode that the settings menu is attached to */
    std::shared_ptr<cugl::scene2::SceneNode> _sceneNode;

    /** Reference to the node containing the entire settings menu */
    std::shared_ptr<cugl::scene2::SceneNode> _settingsMenu;

    // Main settings menu references 
    /** Reference to the main settings menu */
    std::shared_ptr<cugl::scene2::SceneNode> _settingsMainMenu;
    /** Reference to the node containing the button to drop down the settings menu */
    std::shared_ptr<cugl::scene2::SceneNode> _settingsButtonNode;
    /** Reference to the node containing the title of the settings menu */
    std::shared_ptr<cugl::scene2::Label> _settingsTitle;
    /** Reference to the node containing the sounds button */
    std::shared_ptr<cugl::scene2::Button> _soundsButton;
    /** Reference to the node containing the stats button */
    std::shared_ptr<cugl::scene2::Button> _statsButton;
    /** Reference to the node containing the quit button */
    std::shared_ptr<cugl::scene2::Button> _quitButton;
    /** Reference to the node containing the close button */
    std::shared_ptr<cugl::scene2::Button> _closeButton;

    // References to nodes inside the sounds menu
    /** Reerence to the node containing the sound menu */
    std::shared_ptr<cugl::scene2::SceneNode> _settingsSoundMenu;
    /** Reference to the node containing the back button */
    std::shared_ptr<cugl::scene2::Button> _backButton;
    /** Reference to the node containing the title of the sound menu */
    std::shared_ptr<cugl::scene2::Label>  _soundTitle;
    /** Reference to the node containing the title of the music volume slider */
    std::shared_ptr<cugl::scene2::Label>  _musicTitle;
    /** Reference to the node containing the title of the SFX volume slider */
    std::shared_ptr<cugl::scene2::Label>  _SFXTitle;
    /** Reference to the node containing the music volume slider */
    std::shared_ptr<cugl::scene2::Slider> _musicSlider;
    /** Reference to the node containing the SFX volume slider */
    std::shared_ptr<cugl::scene2::Slider> _SFXSlider;

    // Enum to save which state of the settings menu we are inside
    enum State {
        /** The main menu state */
        MAIN,
        /** The sound menu state */
        SOUND,
        /** The stats viewing state */
        STATS,
        /** The calibration state */
        CALIBRATE
    };

    // The state that the settings menu is currently in
    State _currSettingsState;
    // The state that the settings menu was previously in
    State _prevSettingsState;

    //References for the settings menu animations
    /** Reference to the settings menu moving up */
    std::shared_ptr<cugl::scene2::MoveTo> _moveup;
    /** Reference to the settings menu moving up */
    std::shared_ptr<cugl::scene2::MoveTo> _movedn;
    /** Scaled screen dimensions for use in setting position in animations */
    cugl::Size _dimen;

    /** The asset manager for this game mode */
    std::shared_ptr<cugl::AssetManager> _assets;
    /** The action manager for the whole game */
    std::shared_ptr<cugl::scene2::ActionManager> _actions;
    /** The audio controller for the whole game */
    std::shared_ptr<AudioController> _audio;
    /** A reference to the font style for the UI node */
    std::shared_ptr<cugl::Font> _font;
    /** The actual size of the display. */
    cugl::Size _screenSize;
    /** The amount to move the world node by to center it in the scene */
    cugl::Vec2 _offset;
    /** The PolyFactory instance */
    cugl::PolyFactory _pf;
    
    // Information to be sent to GameScene
    /** Whether or not the game has been quit. */
    bool _didQuit;
    /** Whether or not the game is being sent to the pause menu. */
    bool _didPause;
    /** Whether or not the game is paused. */
    bool _isPaused;
    /** Whether or not the game is muted */
    bool _didMute;

    // The transparent color for certain UI elements
    cugl::Color4 _transparent;

public:
//  MARK: - Constructors
    
    /**
     * Constructs a UI Controller
     */
    PauseController() {};
    
    /**
     * Destructs a UI Controller
     */
    ~PauseController() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of UI Controller
     */
    void dispose();
    
    /** Resets the UI Controller */
    void reset();
    
    /**
     * Initializes a UI Controller
     */
    bool init(const std::shared_ptr<cugl::scene2::SceneNode> scenenode,
              cugl::Size screenSize,
              cugl::Vec2 offset,
              const std::shared_ptr<cugl::AssetManager>& assets,
              const std::shared_ptr<cugl::scene2::ActionManager>& actions,
              const std::shared_ptr<AudioController> audio);
        
//  MARK: - Methods
    
    /**
     * Updates the UI Controller
     */
    void update(float timestep);

//  MARK: - Getters

    /**
     * Gets whether or not the game is being quit. 
     */
    bool didQuit() { return _didQuit; }
    /**
     * Gets whether or not the game is being paused.
     */
    bool didPause() { return _didPause; }
    /**
     * Gets whether or not the game is being paused.
     */
    bool isPaused() { return _isPaused; }
    /**
     * Gets whether or not the game is being muted.
     */
    bool didMute() { return _didMute; }


//  MARK: - Setters
    /**
     * Sets whether or not the game is being paused.
     */
    void setDidPause(bool value) { _didPause = value; }

private:
//  MARK: - Helpers
    
    

    /**
     * Performs a move action for the settings menu
     *
     * @param action The move action
     */
    void doMove(const std::shared_ptr<cugl::scene2::MoveTo>& action);

        
};

#endif /* __LCMP_PAUSE_CONTROLLER_H__ */

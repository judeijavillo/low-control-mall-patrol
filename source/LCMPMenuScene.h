//
//  LCMPMenuScene.h
//  Low Control Mall Patrol
//
//  This class presents the initial menu scene.  It allows the player to
//  choose to be a host or a client.
//
//  Originator: Walker White, Aidan Hobler
//  Author: Kevin Games
//  Version: 2/18/22
//

#ifndef __LCMP_MENU_SCENE_H__
#define __LCMP_MENU_SCENE_H__
#include <cugl/cugl.h>
#include <vector>
#include "LCMPAudioController.h"
#include "LCMPPauseController.h"
#include <cugl/scene2/actions/CUActionManager.h>
//#include <cugl/scene2/actions/CUMoveAction.h>
//#include <cugl/scene2/actions/CUScaleAction.h>
//#include <cugl/scene2/actions/CUAnimateAction.h>
//#include <cugl/math/CUEasingBezier.h>

/**
 * This class presents the menu to the player.
 *
 * There is no need for an input controller, as all input is managed by
 * listeners on the scene graph.  We only need getters so that the main
 * application can retrieve the state and communicate it to other scenes.
 */
class MenuScene : public cugl::Scene2 {
public:
//  MARK: - Enumerations
    
    /** The menu choice. */
    enum Choice {
        /** User has not yet made a choice */
        NONE,
        /** User wants to host a game */
        HOST,
        /** User wants to join a game */
        JOIN,
//        /** User wants to find a game */
//        FIND,
        /** User wants to shop */
        SHOP,
        /** User wants to roll gacha */
        GACHA
    };

protected:
//  MARK: - Properties
    
    
    /** The actual size of the display. */
    cugl::Size _screenSize;
    /** The amount to move the world node by to center it in the scene */
    cugl::Vec2 _offset;
    /** The asset manager for this scene. */
    std::shared_ptr<cugl::AssetManager> _assets;
    /** The sound controller for the game */
    std::shared_ptr<AudioController> _audio;
    /** The action manager for the game */
    std::shared_ptr<cugl::scene2::ActionManager> _actions;
    /** The settings menu for the menu */
    PauseController _settings;
    /** The menu button for hosting a game */
    std::shared_ptr<cugl::scene2::Button> _hostbutton;
    /** The menu button for joining a game */
    std::shared_ptr<cugl::scene2::Button> _joinbutton;
    /** The button to open the settings menu */
    std::shared_ptr<cugl::scene2::Button> _settingsButton;
    /** The menu button for entering the shop */
    std::shared_ptr<cugl::scene2::Button> _shopButton;
    /** The menu button for rolling the gacha */
    std::shared_ptr<cugl::scene2::Button> _gachaButton;
    
    std::shared_ptr<cugl::scene2::PolygonNode> _title;

    /** The player menu choice */
    Choice _choice;
    
    cugl::Size _dimen;
    
public:
//  MARK: - Constructors
    /**
     * Creates a new  menu scene with the default values.
     *
     * This constructor does not allocate any objects or start the game.
     * This allows us to use the object without a heap pointer.
     */
    MenuScene() : cugl::Scene2() {}
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     *
     * This method is different from dispose() in that it ALSO shuts off any
     * static resources, like the input controller.
     */
    ~MenuScene() { dispose(); }
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     */
    void dispose() override;
        
    /**
     * Initializes the controller contents.
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
    bool init(const std::shared_ptr<cugl::AssetManager>& assets,
              std::shared_ptr<AudioController>& audio,
              std::shared_ptr<cugl::scene2::ActionManager>& actions,
              bool sixteenNineAspectRatio);

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
    virtual void setActive(bool value) override;
    
    /**
     * Returns the user's menu choice.
     *
     * This will return NONE if the user had no yet made a choice.
     *
     * @return the user's menu choice.
     */
    Choice getChoice() const { return _choice; }
    
};

#endif /* __LCMP_MENU_SCENE_H__ */

//
//  LCMPLevelSelectScene.h
//  Low Control Mall Patrol
//
//  This class presents the level select screen.
//  It allows the host to choose which map they would like to play on.
//
//  Originator: Walker White, Aidan Hobler
//  Author: Kevin Games
//  Version: 2/18/22
//

#ifndef __LCMP_LEVEL_SELECT_SCENE_H__
#define __LCMP_LEVEL_SELECT_SCENE_H__
#include <cugl/cugl.h>
#include <vector>
#include "LCMPAudioController.h"
#include "LCMPNetworkController.h"

/**
 * This class presents the menu to the player.
 *
 * There is no need for an input controller, as all input is managed by
 * listeners on the scene graph.  We only need getters so that the main
 * application can retrieve the state and communicate it to other scenes.
 */
class LevelSelectScene : public cugl::Scene2 {
public:
//  MARK: - Enumerations
    
    /** The menu choice. */
    enum Choice {
        /** User has not yet made a choice */
        NONE,
        /** User wants to play map one */
        ONE,
        /** User wants to play map two */
        TWO,
        /** User wants to play map three */
        THREE,
        /** User wants to play map four */
        FOUR,
        /** User wants to go back to the title screen */
        BACK

    };

protected:
//  MARK: - Properties
    
    /** The amount to move the world node by to center it in the scene */
    cugl::Vec2 _offset;
    /** The asset manager for this scene. */
    std::shared_ptr<cugl::AssetManager> _assets;
    /** A reference to the Network Controller instance */
    std::shared_ptr<NetworkController> _network;
    /** The sound controller for the game */
    std::shared_ptr<AudioController> _audio;
    /** The button for playing on map 1 */
    std::shared_ptr<cugl::scene2::Button> _map1button;
    /** The button for playing on map 2 */
    std::shared_ptr<cugl::scene2::Button> _map2button;
    /** The button for playing on map 3 */
    std::shared_ptr<cugl::scene2::Button> _map3button;
    /** The button for playing on map 4 */
    std::shared_ptr<cugl::scene2::Button> _map4button;
    /** The button for displaying the next two maps */
    std::shared_ptr<cugl::scene2::Button> _nextbutton;
    /** The button for displaying the previous two maps3 */
    std::shared_ptr<cugl::scene2::Button> _prevbutton;
    /** The button for returning to the main menu */
    std::shared_ptr<cugl::scene2::Button> _backbutton;
    /** The player menu choice */
    Choice _choice;

    /** Which map screen the player is looking at */
    int _mapScreen;
    int _prevScreen;
    
public:
//  MARK: - Constructors
    /**
     * Creates a new  menu scene with the default values.
     *
     * This constructor does not allocate any objects or start the game.
     * This allows us to use the object without a heap pointer.
     */
    LevelSelectScene() : cugl::Scene2() {}
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     *
     * This method is different from dispose() in that it ALSO shuts off any
     * static resources, like the input controller.
     */
    ~LevelSelectScene() { dispose(); }
    
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
              std::shared_ptr<NetworkController>& network,
              std::shared_ptr<AudioController>& audio);

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
    
    //  MARK: - Methods

    /**
     * The method called to update the scene.
     *
     * We need to update this method to constantly talk to the server
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void update(float timestep) override;

    /**
     * Returns the user's menu choice.
     *
     * This will return NONE if the user had no yet made a choice.
     *
     * @return the user's menu choice.
     */
    Choice getChoice() const { return _choice; }

    void nextPage();

    void prevPage();

};

#endif /* __LCMP_MENU_SCENE_H__ */

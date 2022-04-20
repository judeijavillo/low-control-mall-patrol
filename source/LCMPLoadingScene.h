//
//  LCMPLoadingScene.h
//  Low Control Mall Patrol
//
//  This module provides a very barebones loading screen.  Most of the time you
//  will not need a loading screen, because the assets will load so fast.  But
//  just in case, this is a simple example you can use in your games.
//
//  We know from 3152 that you all like to customize this screen.  Therefore,
//  we have kept it as simple as possible so that it is easy to modify. In
//  fact, this loading screen uses the new modular JSON format for defining
//  scenes.  See the file "loading.json" for how to change this scene.
//
//  Originator: Walker White, Aidan Hobler
//  Author: Kevin Games
//  Version: 2/18/22
//

#ifndef __LCMP_LOADING_SCENE_H__
#define __LCMP_LOADING_SCENE_H__
#include <cugl/cugl.h>
#include "LCMPAudioController.h"
#include <cugl/scene2/actions/CUActionManager.h>
#include <cugl/scene2/actions/CUAnimateAction.h>

/**
 * This class is a simple loading screen for asychronous asset loading.
 *
 * The screen will display a very minimal progress bar that displays the
 * status of the asset manager.  Make sure that all asychronous load requests
 * are issued BEFORE calling update for the first time, or else this screen
 * will think that asset loading is complete. 
 *
 * Once asset loading is completed, it will display a play button.  Clicking
 * this button will inform the application root to switch to the gameplay mode.
 */
class LoadingScene : public cugl::Scene2 {
protected:
//  MARK: - Properties
    
    /** The asset manager for loading. */
    std::shared_ptr<cugl::AssetManager> _assets;
    /** A reference to the Audio Controller instance */
    std::shared_ptr<AudioController> _audio;
    /** A reference to the Action Manager */
    std::shared_ptr<cugl::scene2::ActionManager> _actions;
    
    // VIEW
    /** The animated progress bar */
    std::shared_ptr<cugl::scene2::ProgressBar>  _bar;
    /** The engine name */
    std::shared_ptr<cugl::scene2::SceneNode>  _brand;
    /** The "play" button */
    std::shared_ptr<cugl::scene2::Button>    _button;


    /** The child node for displaying the loading texture */
    std::shared_ptr<cugl::scene2::SpriteNode> _aniSpriteNode;
    /** The loading screen animation */
    std::shared_ptr<cugl::scene2::Animate> _animation;
    /** The loading screen sprite sheet */
    std::shared_ptr<cugl::Texture> _spriteSheet;
    /** The current animation frame */
    int _aniFrame;
    /** The previous timestep. */
    float _prevTime;

    // MODEL
    /** The progress displayed on the screen */
    float _progress;
    /** Whether or not the player has pressed play to continue */
    bool  _completed;
    
public:
//  MARK: - Constructors
    
    /**
     * Creates a new loading mode with the default values.
     *
     * This constructor does not allocate any objects or start the game.
     * This allows us to use the object without a heap pointer.
     */
    LoadingScene() : cugl::Scene2(), _progress(0.0f) {}
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     *
     * This method is different from dispose() in that it ALSO shuts off any
     * static resources, like the input controller.
     */
    ~LoadingScene() { dispose(); }
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     */
    void dispose();
    
    /**
     * Initializes the controller contents, making it ready for loading
     *
     * The constructor does not allocate any objects or memory.  This allows
     * us to have a non-pointer reference to this controller, reducing our
     * memory allocation.  Instead, allocation happens in this method.
     *
     * @param assets    The (loaded) assets for this game mode
     *
     * @return true if the controller is initialized properly, false otherwise.
     */
    bool init(const std::shared_ptr<cugl::AssetManager>& assets,
              std::shared_ptr<AudioController>& audio,
              std::shared_ptr<cugl::scene2::ActionManager>& actions);

    
//  MARK: - Progress Monitoring
    
    /**
     * The method called to update the game mode.
     *
     * This method updates the progress bar amount.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void update(float timestep);

    /**
     * Returns true if loading is complete, but the player has not pressed play
     *
     * @return true if loading is complete, but the player has not pressed play
     */
    bool isPending() const;

    /**
     * Controls animation
     */
    void playAnimation(float dt);
};

#endif /* __NL_LOADING_SCENE_H__ */

//
//  LCMPLoadingScene.cpp
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

#include "LCMPLoadingScene.h"
#include "LCMPConstants.h"

using namespace cugl;

//  MARK: - Constants

/** This is the ideal size of the logo */
#define SCENE_SIZE  1024

/** The keys for the loading animation */
#define LOADING           "load_animation"

/** Number of frames in animation */
#define ANIMATION_NUM_FRAMES    52

//  MARK: - Constructors

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
bool LoadingScene::init(const std::shared_ptr<AssetManager>& assets,
                        std::shared_ptr<AudioController>& audio,
                        std::shared_ptr<cugl::scene2::ActionManager>& actions) {
    
    // Initialize the scene to a locked width
    Size dimen = Application::get()->getDisplaySize();
    
    // Lock the scene to a reasonable resolution
    if (dimen.width > dimen.height) dimen *= SCENE_SIZE/dimen.width;
    else dimen *= SCENE_SIZE/dimen.height;
    
    // Give up if initialization fails early
    if (assets == nullptr) return false;
    else if (!Scene2::init(dimen)) return false;
    
    // IMMEDIATELY load the splash screen assets
    _assets = assets;
    _assets->loadDirectory("json/loading.json");
    auto layer = assets->get<scene2::SceneNode>("load");
    layer->setContentSize(dimen);
    layer->doLayout(); // This rearranges the children to fit the screen
    
    // Start loading music
    _audio = audio;
    _audio->playSound(_assets, LOADING_MUSIC, false, -1);
    
    // Save the SceneNodes that we'll need to access later
    _brand = assets->get<scene2::SceneNode>("load_name");
    _button = std::dynamic_pointer_cast<scene2::Button>(assets->get<scene2::SceneNode>("load_play"));
    _button->addListener([=](const std::string& name, bool down) {
        this->_active = down;
        if (down) _audio->playSound(_assets, CLICK_SFX, true, 0);
    });

    // Loading screen animation initalization
    _aniSpriteNode = std::dynamic_pointer_cast<scene2::SpriteNode>(assets->get<scene2::SceneNode>(LOADING));
    _aniFrame = 0;
    _prevTime = 0;
    
    // Set the background color and add the LoadingScene to the screen
    Application::get()->setClearColor(Color4(192,192,192,255));
    addChild(layer);
    return true;
}

/**
 * Disposes of all (non-static) resources allocated to this mode.
 */
void LoadingScene::dispose() {
    // Deactivate the button (platform dependent)
    if (isPending()) {
        _button->deactivate();
    }
    _button = nullptr;
    _brand = nullptr;
    _assets = nullptr;
    _progress = 0.0f;
}


//  MARK: - Progress Monitoring

/**
 * The method called to update the game mode.
 *
 * This method updates the progress bar amount.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LoadingScene::update(float timestep) {
    if (_progress < 1) {
        _progress = _assets->progress();
        if (_progress >= 1) {
            _progress = 1.0f;
            _brand->setVisible(false);
            _button->setVisible(true);
            _button->activate();
        }
    }
    playAnimation(timestep);
}

/**
 * Returns true if loading is complete, but the player has not pressed play
 *
 * @return true if loading is complete, but the player has not pressed play
 */
bool LoadingScene::isPending() const {
    return _button != nullptr && _button->isVisible();
}

/**
 * Controls animation
 */
void LoadingScene::playAnimation(float timestep) {
    _prevTime += timestep;
    int f = (int) (_progress * ANIMATION_NUM_FRAMES);
    
    if (_prevTime >= 0.1 && f > _aniFrame) {
        _prevTime = 0;
        _aniFrame++;
        _aniSpriteNode->setFrame(_aniFrame);
    }
}

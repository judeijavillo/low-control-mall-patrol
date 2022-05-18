//
//  LCMPLoadingScene.cpp
//  Low Control Mall Patrol
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

/** Time between animation frames */
#define ANIMATION_SPEED         0.07f

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
                        std::shared_ptr<AudioController>& audio) {
    
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
    _button = std::dynamic_pointer_cast<scene2::Button>(assets->get<scene2::SceneNode>("load_background_play"));
    _button->addListener([=](const std::string& name, bool down) {
        this->_active = down;
        if (down) _audio->playSound(_assets, CLICK_SFX, true, 0);
    });

    // Loading screen animation initalization
    _aniFrame = 0;
    _prevTime = 0;
    _background = std::dynamic_pointer_cast<scene2::PolygonNode>(assets->get<scene2::SceneNode>("load_background"));
    _copNode = std::dynamic_pointer_cast<scene2::SpriteNode>(assets->get<scene2::SceneNode>("load_background_cop"));
    _thiefNode = std::dynamic_pointer_cast<scene2::SpriteNode>(assets->get<scene2::SceneNode>("load_background_thief"));
    _tackleNode = std::dynamic_pointer_cast<scene2::PolygonNode>(assets->get<scene2::SceneNode>("load_background_tackle"));
    _landNode = std::dynamic_pointer_cast<scene2::PolygonNode>(assets->get<scene2::SceneNode>("load_background_land"));
    _thiefNode->setPositionX(SCENE_WIDTH * 0.75);
    _copNode->setScale(0.5);
    _thiefNode->setScale(0.5);
    _tackleNode->setVisible(false);
    _landNode->setVisible(false);
    _tackleNode->setScale(0.5);
    _landNode->setScale(0.5);
    
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
    int copPos = (int) (_progress * 3*SCENE_WIDTH/4);
    int thiefPos = (int) (_progress * SCENE_WIDTH/4 + 3*SCENE_WIDTH/4);
    
    if (copPos >= 3*SCENE_WIDTH/4) {
        _tackleNode->setVisible(false);
        _landNode->setVisible(true);
        _landNode->setPositionX(copPos);
    }
    else if (copPos >= SCENE_WIDTH/2) {
        _copNode->setVisible(false);
        _tackleNode->setVisible(true);
        _tackleNode->setPositionX(copPos);
    }
    
    if (_prevTime >= ANIMATION_SPEED) {
        _prevTime = 0;
        
        _aniFrame = (_aniFrame >= 7) ? 0 : _aniFrame + 1;
        _copNode->setFrame(_aniFrame);
        _thiefNode->setFrame(_aniFrame);
    }
    
    if (copPos > _copNode->getPositionX()) {
        _copNode->setPositionX(copPos);
        _thiefNode->setPositionX(thiefPos);
    }
}

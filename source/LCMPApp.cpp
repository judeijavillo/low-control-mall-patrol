//
//  LCMPApp.cpp
//  Low Control Mall Patrol
//
//  This is the root class for Low Control Mall Patrol.
//
//  Originator: Walker White, Aidan Hobler
//  Author: Kevin Games
//  Version: 2/18/22
//

#include "LCMPApp.h"
#include "LCMPConstants.h"

using namespace cugl;

//  MARK: - Lifecycle

/**
 * The method called after OpenGL is initialized, but before running the application.
 *
 * This is the method in which all user-defined program intialization should
 * take place.  You should not create a new init() method.
 *
 * When overriding this method, you should call the parent method as the
 * very last line.  This ensures that the state will transition to FOREGROUND,
 * causing the application to run.
 */
void LCMPApp::onStartup() {
    _assets = AssetManager::alloc();
    _batch  = SpriteBatch::alloc();
    _actions = scene2::ActionManager::alloc();
    auto cam = OrthographicCamera::alloc(getDisplaySize());
    
    // Start-up basic input
#ifdef CU_MOBILE
    Input::activate<Touchscreen>();
    Input::activate<Accelerometer>();
#else
    Input::activate<Mouse>();
    Input::get<Mouse>()->setPointerAwareness(Mouse::PointerAwareness::DRAG);
#endif
    Input::activate<Keyboard>();
    Input::activate<TextInput>();
    
    // Initialize networking
    _network = make_shared<NetworkController>();
    _network->init();
    
    // Initialize audio
    _audio = make_shared<AudioController>();

    // Attach loaders to the asset manager
    _assets->attach<Font>(FontLoader::alloc()->getHook());
    _assets->attach<Texture>(TextureLoader::alloc()->getHook());
    _assets->attach<JsonValue>(JsonLoader::alloc()->getHook());
    _assets->attach<WidgetValue>(WidgetLoader::alloc()->getHook());
    _assets->attach<scene2::SceneNode>(Scene2Loader::alloc()->getHook());
    _assets->attach<Sound>(SoundLoader::alloc()->getHook());
    
    // Queue up the other assets
    _assets->loadDirectoryAsync("json/assets.json", nullptr);
    _assets->loadDirectoryAsync("json/host.json",nullptr);
    _assets->loadDirectoryAsync("json/join.json",nullptr);
    _assets->loadDirectoryAsync("json/find.json",nullptr);
    _assets->loadDirectoryAsync("json/customize.json",nullptr);
    _assets->loadDirectoryAsync("json/levelselect.json", nullptr);
    _assets->loadDirectoryAsync("json/skins.json",nullptr);
    _assets->loadDirectoryAsync("json/victory.json",nullptr);
    _assets->loadDirectoryAsync(WALL_ASSETS_FILE, nullptr);
    
    // Create a "loading" screen
    _scene = State::LOAD;
    _loading.init(_assets, _audio, _actions);

    // Initialize fade status
    _fadeStatus = FadeStatus::FADE_WHITE;
    
    // Call the parent's onStartup
    Application::onStartup();
}

/**
 * The method called when the application is ready to quit.
 *
 * This is the method to dispose of all resources allocated by this
 * application.  As a rule of thumb, everything created in onStartup()
 * should be deleted here.
 *
 * When overriding this method, you should call the parent method as the
 * very last line.  This ensures that the state will transition to NONE,
 * causing the application to be deleted.
 */
void LCMPApp::onShutdown() {
    _game.disconnect();
    _host.disconnect();
    _client.disconnect();
    _find.disconnect();
    
    _loading.dispose();
    _game.dispose();
    _audio->dispose();
    _actions = nullptr;
    _assets = nullptr;
    _batch = nullptr;

    // Shutdown input
#ifdef CU_MOBILE
    Input::deactivate<Touchscreen>();
    Input::deactivate<Accelerometer>();
#else
    Input::deactivate<Mouse>();
#endif
    Input::deactivate<TextInput>();
    Input::deactivate<Keyboard>();

    // Call the parent's onShutdown
    Application::onShutdown();
}

/**
 * The method called to update the application data.
 *
 * This is your core loop and should be replaced with your custom implementation.
 * This method should contain any code that is not an OpenGL call.
 *
 * When overriding this method, you do not need to call the parent method
 * at all. The default implmentation does nothing.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::update(float timestep) {
    switch (_scene) {
    case LOAD:
        updateLoadingScene(timestep);
        break;
    case MENU:
        updateMenuScene(timestep);
        break;
    case HOST:
        updateHostScene(timestep);
        break;
    case CLIENT:
        updateClientScene(timestep);
        break;
    case FIND:
        updateFindScene(timestep);
        break;
    case CUSTOM:
        updateCustomizeScene(timestep);
        break;
    case LEVEL:
        updateLevelSelectScene(timestep);
        break;
    case GAME:
        updateGameScene(timestep);
        break;
    case VICTORY:
        updateVictoryScene(timestep);
        break;
    }
}

/**
 * The method called to draw the application to the screen.
 *
 * This is your core loop and should be replaced with your custom implementation.
 * This method should OpenGL and related drawing calls.
 *
 * When overriding this method, you do not need to call the parent method
 * at all. The default implmentation does nothing.
 */
void LCMPApp::draw() {
    switch (_scene) {
    case LOAD:
        _loading.render(_batch);
        break;
    case MENU:
        _menu.render(_batch);
        break;
    case HOST:
        _host.render(_batch);
        break;
    case CLIENT:
        _client.render(_batch);
        break;
    case FIND:
        _find.render(_batch);
        break;
    case CUSTOM:
        _customize.render(_batch);
        break;
    case LEVEL:
        _levelselect.render(_batch);
        break;
    case GAME:
        _game.render(_batch);
        break;
    case VICTORY:
        _victory.render(_batch);
        break;
    }
}

//  MARK: - Helpers

/**
 * Inidividualized update method for the loading scene.
 *
 * This method keeps the primary {@link #update} from being a mess of switch
 * statements. It also handles the transition logic from the loading scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::updateLoadingScene(float timestep) {
    if (_loading.isActive()) {
        _loading.update(timestep);
    } else {
        // Leave loading for good, initialize all other scenes
        _loading.dispose();
        _audio->stopMusic(LOADING_MUSIC);
        _menu.init(_assets, _audio, _actions);
        _host.init(_assets, _network, _audio);
        _client.init(_assets, _network, _audio);
        _find.init(_assets, _network);
        _customize.init(_assets, _network, _audio, _actions);
        _levelselect.init(_assets, _audio);
        _game.init(_assets, _network, _audio, _actions);
        _victory.init(_assets, _network, _audio, _actions, true);
        _menu.setActive(true);
        _scene = State::MENU;
    }
}


/**
 * Individualized update method for the menu scene.
 *
 * This method keeps the primary {@link #update} from being a mess of switch
 * statements. It also handles the transition logic from the menu scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::updateMenuScene(float timestep) {
    _menu.update(timestep);
    switch (_menu.getChoice()) {
    case MenuScene::Choice::HOST:
        _menu.setActive(false);
        _host.setActive(true);
        _network->setHost(true);
        _scene = State::HOST;
        break;
    case MenuScene::Choice::JOIN:
        _menu.setActive(false);
        _client.setActive(true);
        _network->setHost(false);
        _scene = State::CLIENT;
        break;
    case MenuScene::Choice::FIND:
        _menu.setActive(false);
        _find.setActive(true);
        _network->setHost(true);
        _scene = State::FIND;
        break;
    case MenuScene::Choice::NONE:
        // DO NOTHING
        break;
    }
}

/**
 * Individualized update method for the level select scene.
 *
 * This method keeps the primary {@link #update} from being a mess of switch
 * statements. It also handles the transition logic from the menu scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::updateLevelSelectScene(float timestep) {
    _levelselect.update(timestep);
    switch (_levelselect.getChoice()) {
    case LevelSelectScene::Choice::ONE:
        _levelKey = LEVEL_ONE_FILE;
        _levelselect.setActive(false);
        _game.setActive(true);
        _scene = State::GAME;
        _network->sendStartGame();
        _game.start(true, _customize.skinKey, LEVEL_DONUT_KEY);
        break;
    case LevelSelectScene::Choice::TWO:
        _levelKey = LEVEL_ONE_FILE;
        _levelselect.setActive(false);
        _game.setActive(true);
        _scene = State::GAME;
        _network->sendStartGame();
        _game.start(true, _customize.skinKey, LEVEL_CONVEYOR_KEY);
        break;
    case LevelSelectScene::Choice::THREE:
        _levelKey = LEVEL_ONE_FILE;
        _levelselect.setActive(false);
        _game.setActive(true);
        _scene = State::GAME;
        _network->sendStartGame();
        _game.start(true, _customize.skinKey, _levelKey);
        break;
    case LevelSelectScene::Choice::FOUR:
        _levelKey = LEVEL_ONE_FILE;
        _levelselect.setActive(false);
        _game.setActive(true);
        _scene = State::GAME;
        _network->sendStartGame();
        _game.start(true, _customize.skinKey, _levelKey);
        break;
    case LevelSelectScene::Choice::BACK:
        _levelselect.setActive(false);
        _menu.setActive(true);
        _scene = State::MENU;
        break;
    case LevelSelectScene::Choice::NONE:
        // DO NOTHING
        break;
    }
}


/**
 * Individualized update method for the host scene.
 *
 * This method keeps the primary {@link #update} from being a mess of switch
 * statements. It also handles the transition logic from the host scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::updateHostScene(float timestep) {
    _host.update(timestep);
    // TODO: Make changes with factored out Network Controller
    switch (_host.getStatus()) {
    case HostScene::Status::ABORT:
        _host.setActive(false);
        _menu.setActive(true);
        _scene = State::MENU;
        break;
    case HostScene::Status::START:
        _host.setActive(false);
        _customize.setActive(true, true);
        _scene = State::CUSTOM;
        break;
    case HostScene::Status::WAIT:
    case HostScene::Status::IDLE:
        // DO NOTHING
        break;
    }
}

/**
 * Individualized update method for the client scene.
 *
 * This method keeps the primary {@link #update} from being a mess of switch
 * statements. It also handles the transition logic from the client scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::updateClientScene(float timestep) {
    _client.update(timestep);
    // TODO: Make changes with factored out Network Controller
    switch (_client.getStatus()) {
    case ClientScene::Status::ABORT:
        _client.setActive(false);
        _menu.setActive(true);
        _scene = State::MENU;
        break;
    case ClientScene::Status::START:
        _client.setActive(false);
        _game.setActive(true);
        _scene = State::GAME;
        _game.start(false, _customize.skinKey, LEVEL_ONE_FILE);
        break;
    case ClientScene::Status::WAIT:
    case ClientScene::Status::IDLE:
    case ClientScene::Status::JOIN:
        // DO NOTHING
        break;
    }
}

/**
 * Individualized update method for the room scene.
 *
 * This method keeps the primary {@link #update} from being a mess of switch
 * statements. It also handles the transition logic from the room scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::updateFindScene(float timestep) {
    _find.update(timestep);
    switch (_find.getStatus()) {
    case FindScene::Status::ABORT:
        _find.setActive(false);
        _menu.setActive(true);
        _scene = State::MENU;
        break;
    case FindScene::Status::START:
        _find.setActive(false);
        _game.setActive(true);
        _scene = State::GAME;
        _game.start(true, _customize.skinKey, _levelKey);
//        _customize.setActive(true);
//        _scene = State::CUSTOM;
        break;
    case FindScene::Status::WAIT:
    case FindScene::Status::IDLE:
    case FindScene::Status::HOST:
    case FindScene::Status::CLIENT:
        // DO NOTHING
        break;
    }
}

/**
 * Individualized update method for the customization scene.
 *
 * This method keeps the primary {@link #update} from being a mess of switch
 * statements. It also handles the transition logic from the host scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::updateCustomizeScene(float timestep) {
    _customize.update(timestep);
    // TODO: Make changes with factored out Network Controller
    switch (_customize.getStatus()) {
    case CustomizeScene::Status::ABORT:
        _customize.setActive(false, true);
        _menu.setActive(true);
        _scene = State::MENU;
        break;
    case CustomizeScene::Status::START:
        _customize.setActive(false, true);
        if (_network->isHost()) {
            _levelselect.setActive(true);
            _scene = State::LEVEL;
        }
        break;
    case CustomizeScene::Status::WAIT:
    case CustomizeScene::Status::IDLE:
        // DO NOTHING
        break;
    }
}

/**
 * Individualized update method for the game scene.
 *
 * This method keeps the primary {@link #update} from being a mess of switch
 * statements. It also handles the transition logic from the game scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::updateGameScene(float timestep) {
    _game.update(timestep);
    if (_game.didQuit()) {
        _game.setActive(false);
        _menu.setActive(true);
        _scene = State::MENU;
        
        _audio->stopMusic(GAME_MUSIC);
        _audio->playSound(_assets, MENU_MUSIC, false, -1);
    }
    if (_game.getState() == GameScene::State::DONE) {
        _game.setActive(false);
        _scene = State::VICTORY;
        _victory.setActive(true, _game.isThief(), _game.isThiefWin());
    }
}

/**
 * Individualized update method for the victory scene.
 *
 * This method keeps the primary {@link #update} from being a mess of switch
 * statements. It also handles the transition logic from the game scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void LCMPApp::updateVictoryScene(float timestep) {
    _victory.update(timestep);
    switch (_victory.getStatus()) {
        case VictoryScene::Status::ABORT:
            _victory.setActive(false, false, false);
            _game.dispose(); // bug where settings menu is not removed
            _menu.setActive(true);
            _scene = State::MENU;
            break;
        case VictoryScene::Status::START:
            _victory.setActive(false, false, false);
            _game.reset();
            _game.setActive(true);
            _scene = State::GAME;
            break;
        case VictoryScene::Status::WAIT:
        case VictoryScene::Status::IDLE:
            break;
    }
}

/**
 * Transition between two scenes.
 *
 * This method fades out the previous scene, and fades in the next scene.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 * @param prevScene The scene we are transitioning from
 * @param newScene  The scene we are transitioning into
 */
bool LCMPApp::transitionScene(float timestep, cugl::Scene2 prevScene, cugl::Scene2 newScene) {
    float transitionTime = 0.25f;
    switch (_fadeStatus) {
    case FadeStatus::FADE_WHITE:
        _fadingCumTime = 0;
        _fadeStatus = FadeStatus::FADE_OUT;
        break;
    case FadeStatus::FADE_OUT:
        if (fadeScene(timestep, prevScene, false))
            _fadeStatus = FadeStatus::FADE_BLACK;
        break;
    case FadeStatus::FADE_BLACK:
        prevScene.setActive(false);
        newScene.setColor(Color4::BLACK);
        newScene.setActive(true);
        _fadingCumTime = 0;
        _fadeStatus = FADE_IN;
        break;
    case FadeStatus::FADE_IN:
        if (fadeScene(timestep, newScene, true)) {
            _fadeStatus = FadeStatus::FADE_WHITE;
            return true;
        }
        break;
    default:
        _fadeStatus = FadeStatus::FADE_OUT;
        break;
    }
    return false;
}

/**
 * Fade out a scene.
 *
 * This method fades out the input scene to black.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 * @param scene The scene we are fading out.
 */
bool LCMPApp::fadeScene(float timestep, cugl::Scene2 scene, bool inOut) {
    float fadeTime = 3.0f;
    _fadingCumTime += timestep;
    float fadingAlpha = inOut ? (_fadingCumTime / fadeTime)
        : (1 - (_fadingCumTime / fadeTime));
    fadingAlpha = clamp(fadingAlpha, 0.0f, 1.0f);
    Color4 fadeColor = (255 * fadingAlpha, 255 * fadingAlpha, 255 * fadingAlpha, 255);
    scene.setColor(fadeColor);
    bool returnValue = inOut ? fadingAlpha >= 1 : fadingAlpha <= 0;
    return returnValue;
}

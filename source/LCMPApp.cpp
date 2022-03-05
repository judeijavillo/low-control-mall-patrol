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

    // Attach loaders to the asset manager
    _assets->attach<Font>(FontLoader::alloc()->getHook());
    _assets->attach<Texture>(TextureLoader::alloc()->getHook());
    _assets->attach<JsonValue>(JsonLoader::alloc()->getHook());
    _assets->attach<WidgetValue>(WidgetLoader::alloc()->getHook());
    _assets->attach<scene2::SceneNode>(Scene2Loader::alloc()->getHook());

    // Create a "loading" screen
    _scene = State::LOAD;
    _loading.init(_assets);
    
    // Queue up the other assets
    _assets->loadDirectoryAsync("json/assets.json",nullptr);
    
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
    
    _loading.dispose();
    _game.dispose();
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
    case GAME:
        updateGameScene(timestep);
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
    case GAME:
        _game.render(_batch);
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
        _menu.init(_assets);
        _host.init(_assets, _network);
        _client.init(_assets, _network);
        _game.init(_assets, _network);
        _menu.setActive(true);
        _scene = State::MENU;
    }
}


/**
 * Inidividualized update method for the menu scene.
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
    case MenuScene::Choice::NONE:
        // DO NOTHING
        break;
    }
}

/**
 * Inidividualized update method for the host scene.
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
        _game.setActive(true);
        _scene = State::GAME;
        _game.start(true);
        break;
    case HostScene::Status::WAIT:
    case HostScene::Status::IDLE:
        // DO NOTHING
        break;
    }
}

/**
 * Inidividualized update method for the client scene.
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
        _game.start(false);
        break;
    case ClientScene::Status::WAIT:
    case ClientScene::Status::IDLE:
    case ClientScene::Status::JOIN:
        // DO NOTHING
        break;
    }
}

/**
 * Inidividualized update method for the game scene.
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
    }
}


